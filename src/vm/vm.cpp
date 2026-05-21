/**
 * @file vm.cpp
 * @brief Lua虚拟机执行引擎实现 — 纯自由函数风格
 *
 * 设计说明：
 * 与 Lua 5.1 C 实现的 lvm.c 完全对齐：
 * - luaV_execute(L, nexeccalls) 是自由函数
 * - pc, base, cl, k 全部作为局部变量存在于函数调用栈中
 * - 辅助操作（gettable, settable, arith, concat 等）是独立的自由函数，
 *   仅接收 LuaState* 和具体的值参数
 * - 无任何类或对象——所有执行状态显式传递
 *
 * 参考：lua_c_analysis/src/lvm.c luaV_execute()
 */

#include "vm/vm.hpp"
#include "common/lua_error.hpp"
#include "vm/vm_constants.hpp"
#include "vm/vm_dispatch.hpp"
#include "vm/vm_dispatch_strategy.hpp"
#include "vm/vm_handlers.hpp"
#include "vm/vm_internal.hpp"
#include "vm/lua_state.hpp"
#include "core/value.hpp"
#include "core/function.hpp"
#include "core/table.hpp"
#include "core/gc_string.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "core/metatable.hpp"
#include "vm/global_state.hpp"
#include "runtime/runtime_services.hpp"
#include "compiler/opcode.hpp"

namespace Lua {

// =====================================================================
// 匿名命名空间：内部辅助自由函数
// =====================================================================

namespace {

// -----------------------------------------------------------------
// 基础工具函数
// -----------------------------------------------------------------

/**
 * @brief 刷新 base 指针（栈可能因扩展而重新分配）
 * 对应 Lua C: base = L->base;
 */
static inline Value* refreshBase(LuaState* L) {
    return &L->getStack()[L->getCurrentCallInfo().base];
}

} // anonymous namespace

// =====================================================================
// 公共 API：namespace VM
// =====================================================================

namespace VM {

// -----------------------------------------------------------------
// VM::executeProto — 主执行循环
// 局部变量：func, proto, pc, base（与 Lua C luaV_execute 一致）
// -----------------------------------------------------------------

ExecResult executeProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    return executeProto(services, L, proto, nexeccalls);
}

ExecResult executeProto(RuntimeServices& services, LuaState* L, Proto* proto, i32 nexeccalls) {
    if (!proto)
        throw RuntimeError("VM::executeProto: null proto");
    if (nexeccalls >= MAX_CALLS)
        throw MemoryError("VM: stack overflow (too many nested calls)");

    VMContext context{services, L, proto, nexeccalls};
    DispatchStrategy& strategy = services.dispatchStrategy != nullptr
                               ? *services.dispatchStrategy
                               : defaultDispatchStrategy();
    return strategy.run(context);
}

namespace {

enum class DispatchBackend : u8 {
    Switch,
    Table,
};

ExecResult runDispatchBackend(VMContext& context, DispatchBackend backend) {
    RuntimeServices& services = context.services;
    LuaState* L = context.state;
    Proto* proto = context.proto;
    i32 nexeccalls = context.nexeccalls;

    // 深度检查
    if (!proto)
        throw RuntimeError("VM::executeProto: null proto");
    if (nexeccalls >= MAX_CALLS)
        throw MemoryError("VM: stack overflow (too many nested calls)");

    // ---- 局部执行状态 ----
    Function* func  = nullptr;
    Value*    base  = nullptr;
    usize     pc    = 0;

reentry: // ⭐ 重入点：从 CallInfo 恢复所有执行状态
    {
        CallInfo& ci = L->getCurrentCallInfo();
        Stack& stack = L->getStack();

        // 恢复 func
        if (ci.func >= stack.capacity())
            throw RuntimeError("VM::executeProto: CallInfo.func out of range");
        Value& funcVal = stack[ci.func];
        if (!funcVal.isFunction())
            throw RuntimeError("VM::executeProto: CallInfo.func is not a function");
        func = funcVal.asFunction();

        // 恢复 proto 和 pc
        proto = func->getProto();
        pc = ci.savedpc
           ? static_cast<usize>(ci.savedpc - proto->getCode().data())
           : 0;

        // dump bytecode at entry
        if (VM::detail::shouldDumpBytecode())
        {
            const Vec<Instruction>& dcode = proto->getCode();
            std::fprintf(stderr, "[BCDUMP] proto(%p) %zu instructions, pc=%zu\n",
                (void*)proto, dcode.size(), pc);
            for (usize di = 0; di < dcode.size(); di++) {
                Instruction dinst = dcode[di];
                std::fprintf(stderr, "  [%zu] op=%d A=%d B=%d C=%d Bx=%d sBx=%d\n",
                    di, static_cast<int>(GET_OPCODE(dinst)),
                    GETARG_A(dinst), GETARG_B(dinst), GETARG_C(dinst),
                    GETARG_Bx(dinst), GETARG_sBx(dinst));
            }
        }


        // 确保栈空间
        usize requiredTop = ci.base + proto->getMaxStackSize();
        if (stack.capacity() < requiredTop)
            stack.checkSpace(requiredTop - stack.size());
        while (stack.size() < requiredTop)
            stack.push(Value());

        // 刷新 base
        base = &stack[ci.base];
    }

    // ---- 主执行循环 ----
    {
        const Vec<Instruction>& code = proto->getCode();

        while (pc < code.size()) {
            usize instructionPc = pc;
            Instruction inst = code[pc];
            OpCode op  = GET_OPCODE(inst);
            pc++;

            CallInfo& currentCI = L->getCurrentCallInfo();
            currentCI.savedpc = code.data() + pc;

            VM::detail::dispatchCountHook(L);
            base = refreshBase(L);
            VM::detail::dispatchLineHook(L, proto, instructionPc);
            base = refreshBase(L);

            VM::detail::emitInstructionTrace(proto, base, instructionPc, inst, nexeccalls);

            auto runCurrentHandler = [&]() -> HandlerStatus {
                OpExecutionContext opContext{
                    services,
                    L,
                    func,
                    proto,
                    base,
                    pc,
                    instructionPc,
                    nexeccalls
                };
                HandlerStatus status = VM::runHandler(opContext, inst);
                base = opContext.base;
                nexeccalls = opContext.nexeccalls;
                return status;
            };

            if (backend == DispatchBackend::Table) {
                HandlerStatus status = runCurrentHandler();
                switch (status) {
                    case HandlerStatus::Continue:
                        continue;
                    case HandlerStatus::Reenter:
                        goto reentry;
                    case HandlerStatus::Yielded:
                        return ExecResult::Yielded;
                    case HandlerStatus::Returned:
                        return ExecResult::Returned;
                }
            }

            switch (op) {

            case OpCode::MOVE:
            case OpCode::LOADK:
            case OpCode::LOADBOOL:
            case OpCode::LOADNIL:
            case OpCode::GETGLOBAL:
            case OpCode::SETGLOBAL:
            case OpCode::GETUPVAL:
            case OpCode::SETUPVAL:
            case OpCode::GETTABLE:
            case OpCode::SETTABLE:
            case OpCode::NEWTABLE:
            case OpCode::SELF:
            case OpCode::SETLIST:
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::MOD:
            case OpCode::POW:
            case OpCode::UNM:
            case OpCode::NOT:
            case OpCode::LEN:
            case OpCode::CONCAT:
            case OpCode::JMP:
            case OpCode::EQ:
            case OpCode::LT:
            case OpCode::LE:
            case OpCode::TEST:
            case OpCode::TESTSET:
            case OpCode::CALL:
            case OpCode::TAILCALL:
            case OpCode::RETURN:
            case OpCode::CLOSE:
            case OpCode::FORLOOP:
            case OpCode::FORPREP:
            case OpCode::TFORLOOP:
            case OpCode::CLOSURE:
            case OpCode::VARARG: {
                HandlerStatus status = runCurrentHandler();
                switch (status) {
                    case HandlerStatus::Continue:
                        break;
                    case HandlerStatus::Reenter:
                        goto reentry;
                    case HandlerStatus::Yielded:
                        return ExecResult::Yielded;
                    case HandlerStatus::Returned:
                        return ExecResult::Returned;
                }
                break;
            }

            // ============== 未知指令 ==============

            default:
                throw RuntimeError("VM: unsupported opcode: "
                                   + Str(getOpName(op)));

            } // switch
        } // while
    } // scope for code reference

    return ExecResult::Returned;
}

}  // namespace

ExecResult SwitchDispatch::run(VMContext& context) {
    return runDispatchBackend(context, DispatchBackend::Switch);
}

ExecResult TableDispatch::run(VMContext& context) {
    return runDispatchBackend(context, DispatchBackend::Table);
}

} // namespace VM

} // namespace Lua
