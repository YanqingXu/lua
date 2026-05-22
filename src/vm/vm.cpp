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
#include "vm/vm_switch_dispatch.hpp"
#include "vm/state/lua_state.hpp"
#include "core/value.hpp"
#include "core/function.hpp"
#include "core/table.hpp"
#include "core/gc_string.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "core/metatable.hpp"
#include "vm/state/global_state.hpp"
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

namespace {

ExecResult executeProtoUnchecked(RuntimeServices& services, LuaState* L, Proto* proto, i32 nexeccalls) {
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

}  // namespace

ExecResult executeProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    return executeProto(services, L, proto, nexeccalls);
}

ExecResult executeProto(RuntimeServices& services, LuaState* L, Proto* proto, i32 nexeccalls) {
    return executeProtoUnchecked(services, L, proto, nexeccalls);
}

std::expected<ExecResult, RuntimeError> tryExecuteProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    return tryExecuteProto(services, L, proto, nexeccalls);
}

std::expected<ExecResult, RuntimeError> tryExecuteProto(
    RuntimeServices& services, LuaState* L, Proto* proto, i32 nexeccalls) {
    return VM::detail::captureRuntimeErrors<ExecResult>([&]() {
        return executeProtoUnchecked(services, L, proto, nexeccalls);
    });
}

namespace {

enum class DispatchBackend : u8 {
    Switch,
    Table,
};

// Dispatch timing note:
//
// runDispatchBackend keeps a next-pc invariant. After fetch, `pc` immediately
// points at the next instruction; branch, TEST-skip, CALL reentry, and table
// handlers all adjust that next-pc value. `CallInfo::savedpc` stores the same
// recoverable position before any debug/count/line hook runs, while
// `instructionPc` remains the current instruction for line hooks and tracing.
// New Lua frames have no saved position yet, so a null savedpc starts at PC 0.
// Count hooks run before line hooks; either hook may touch the stack or call
// info, so `base` is refreshed after each hook before executing the opcode.
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

        proto = func->getProto();
        const auto entryCode = proto->getInstructionSpan();
        pc = ci.savedpc
           ? static_cast<usize>(ci.savedpc - entryCode.data())
           : 0;

        // dump bytecode at entry
        if (VM::detail::shouldDumpBytecode())
        {
            const auto dcode = proto->getInstructionSpan();
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
        const auto code = proto->getInstructionSpan();

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

            const bool traceDiff = VM::isTraceDiffEnabled() && VM::getTraceSink() != nullptr;
            const usize traceFrameBase = L->getCurrentCallInfo().base;
            const i32 traceCallDepth = nexeccalls;
            Vec<Value> traceBefore;
            if (traceDiff) {
                traceBefore = VM::detail::captureTraceRegisters(L, traceFrameBase, proto->getMaxStackSize());
            } else {
                VM::detail::emitInstructionTrace(proto, base, instructionPc, inst, nexeccalls);
            }

            if (backend == DispatchBackend::Table) {
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
                if (traceDiff) {
                    VM::detail::emitInstructionTraceDiff(
                        proto, L, traceFrameBase, instructionPc, inst, traceCallDepth, traceBefore);
                }
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

#define LUA_VM_RUN_SWITCH_OP(handlerName)                                      \
    do {                                                                       \
        OpExecutionContext opContext{                                          \
            services, L, func, proto, base, pc, instructionPc, nexeccalls      \
        };                                                                     \
        HandlerStatus status = VM::detail::handlerName(opContext, inst);       \
        base = opContext.base;                                                 \
        nexeccalls = opContext.nexeccalls;                                     \
        if (traceDiff) {                                                       \
            VM::detail::emitInstructionTraceDiff(                              \
                proto, L, traceFrameBase, instructionPc, inst,                 \
                traceCallDepth, traceBefore);                                  \
        }                                                                      \
        switch (status) {                                                      \
            case HandlerStatus::Continue:                                      \
                continue;                                                      \
            case HandlerStatus::Reenter:                                       \
                goto reentry;                                                  \
            case HandlerStatus::Yielded:                                       \
                return ExecResult::Yielded;                                    \
            case HandlerStatus::Returned:                                      \
                return ExecResult::Returned;                                   \
        }                                                                      \
    } while (false)

            switch (op) {

            case OpCode::MOVE:
                LUA_VM_RUN_SWITCH_OP(execOpMove);
                break;
            case OpCode::LOADK:
                LUA_VM_RUN_SWITCH_OP(execOpLoadK);
                break;
            case OpCode::LOADBOOL:
                LUA_VM_RUN_SWITCH_OP(execOpLoadBool);
                break;
            case OpCode::LOADNIL:
                LUA_VM_RUN_SWITCH_OP(execOpLoadNil);
                break;
            case OpCode::GETGLOBAL:
                LUA_VM_RUN_SWITCH_OP(execOpGetGlobal);
                break;
            case OpCode::SETGLOBAL:
                LUA_VM_RUN_SWITCH_OP(execOpSetGlobal);
                break;
            case OpCode::GETUPVAL:
                LUA_VM_RUN_SWITCH_OP(execOpGetUpval);
                break;
            case OpCode::SETUPVAL:
                LUA_VM_RUN_SWITCH_OP(execOpSetUpval);
                break;
            case OpCode::GETTABLE:
                LUA_VM_RUN_SWITCH_OP(execOpGetTable);
                break;
            case OpCode::SETTABLE:
                LUA_VM_RUN_SWITCH_OP(execOpSetTable);
                break;
            case OpCode::NEWTABLE:
                LUA_VM_RUN_SWITCH_OP(execOpNewTable);
                break;
            case OpCode::SELF:
                LUA_VM_RUN_SWITCH_OP(execOpSelf);
                break;
            case OpCode::SETLIST:
                LUA_VM_RUN_SWITCH_OP(execOpSetList);
                break;
            case OpCode::ADD:
                LUA_VM_RUN_SWITCH_OP(execOpAdd);
                break;
            case OpCode::SUB:
                LUA_VM_RUN_SWITCH_OP(execOpSub);
                break;
            case OpCode::MUL:
                LUA_VM_RUN_SWITCH_OP(execOpMul);
                break;
            case OpCode::DIV:
                LUA_VM_RUN_SWITCH_OP(execOpDiv);
                break;
            case OpCode::MOD:
                LUA_VM_RUN_SWITCH_OP(execOpMod);
                break;
            case OpCode::POW:
                LUA_VM_RUN_SWITCH_OP(execOpPow);
                break;
            case OpCode::UNM:
                LUA_VM_RUN_SWITCH_OP(execOpUnm);
                break;
            case OpCode::NOT:
                LUA_VM_RUN_SWITCH_OP(execOpNot);
                break;
            case OpCode::LEN:
                LUA_VM_RUN_SWITCH_OP(execOpLen);
                break;
            case OpCode::CONCAT:
                LUA_VM_RUN_SWITCH_OP(execOpConcat);
                break;
            case OpCode::JMP:
                LUA_VM_RUN_SWITCH_OP(execOpJmp);
                break;
            case OpCode::EQ:
                LUA_VM_RUN_SWITCH_OP(execOpEq);
                break;
            case OpCode::LT:
                LUA_VM_RUN_SWITCH_OP(execOpLt);
                break;
            case OpCode::LE:
                LUA_VM_RUN_SWITCH_OP(execOpLe);
                break;
            case OpCode::TEST:
                LUA_VM_RUN_SWITCH_OP(execOpTest);
                break;
            case OpCode::TESTSET:
                LUA_VM_RUN_SWITCH_OP(execOpTestSet);
                break;
            case OpCode::CALL:
                LUA_VM_RUN_SWITCH_OP(execOpCall);
                break;
            case OpCode::TAILCALL:
                LUA_VM_RUN_SWITCH_OP(execOpTailCall);
                break;
            case OpCode::RETURN:
                LUA_VM_RUN_SWITCH_OP(execOpReturn);
                break;
            case OpCode::CLOSE:
                LUA_VM_RUN_SWITCH_OP(execOpClose);
                break;
            case OpCode::FORLOOP:
                LUA_VM_RUN_SWITCH_OP(execOpForLoop);
                break;
            case OpCode::FORPREP:
                LUA_VM_RUN_SWITCH_OP(execOpForPrep);
                break;
            case OpCode::TFORLOOP:
                LUA_VM_RUN_SWITCH_OP(execOpTForLoop);
                break;
            case OpCode::CLOSURE:
                LUA_VM_RUN_SWITCH_OP(execOpClosure);
                break;
            case OpCode::VARARG: {
                LUA_VM_RUN_SWITCH_OP(execOpVararg);
                break;
            }

            // ============== 未知指令 ==============

            default:
                throw RuntimeError("VM: unsupported opcode: "
                                   + Str(getOpName(op)));

            } // switch

#undef LUA_VM_RUN_SWITCH_OP
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
