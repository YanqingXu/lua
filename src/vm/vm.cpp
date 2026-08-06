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
#include "debugger/debug_runtime.hpp"

#include <cassert>

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

} // namespace

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
    services.globalState.requireOwnerThread();
    if (L == nullptr) {
        throw RuntimeError("VM::executeProto: null state");
    }
    assert(&L->getGlobalState() == &services.globalState);
    if (&L->getGlobalState() != &services.globalState) {
        throw RuntimeError("VM::executeProto: runtime services do not own state");
    }
    if (!proto)
        throw RuntimeError("VM::executeProto: null proto");
    if (nexeccalls >= MAX_CALLS)
        throw StackOverflowError("VM: stack overflow (too many nested calls)");

    if (services.debugger != nullptr) [[unlikely]] {
        services.debugger->registerProto(*proto);
    }

    VMContext context{services, L, proto, nexeccalls};
    DispatchStrategy& strategy =
        services.dispatchStrategy != nullptr ? *services.dispatchStrategy : defaultDispatchStrategy();
    return strategy.run(context);
}

} // namespace

ExecResult executeProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    if (L == nullptr) {
        throw RuntimeError("VM::executeProto: null state");
    }
    RuntimeServices services(L->getGlobalState());
    return executeProto(services, L, proto, nexeccalls);
}

ExecResult executeProto(RuntimeServices& services, LuaState* L, Proto* proto, i32 nexeccalls) {
    try {
        return executeProtoUnchecked(services, L, proto, nexeccalls);
    } catch (const MemoryError& error) {
        if (services.debugger != nullptr &&
            services.debugger->exceptionSafepoint(*L, Debugger::DebugExceptionCategory::ResourceError, error.what(),
                                                  error.hasErrorObject() ? &error.getErrorObject() : nullptr) ==
                Debugger::DebugSafepointResult::TerminateExecution) [[unlikely]] {
            throw RuntimeError("debugger requested execution termination");
        }
        throw;
    } catch (const StackOverflowError& error) {
        if (services.debugger != nullptr &&
            services.debugger->exceptionSafepoint(*L, Debugger::DebugExceptionCategory::ResourceError, error.what(),
                                                  error.hasErrorObject() ? &error.getErrorObject() : nullptr) ==
                Debugger::DebugSafepointResult::TerminateExecution) [[unlikely]] {
            throw RuntimeError("debugger requested execution termination");
        }
        throw;
    } catch (const RuntimeError& error) {
        Debugger::DebugExceptionCategory category = Debugger::DebugExceptionCategory::RuntimeError;
        const ExecutionStopReason stopReason = services.globalState.getExecutionPolicy().lastStopReason();
        if (stopReason == ExecutionStopReason::Cancelled) {
            category = Debugger::DebugExceptionCategory::HostCancellation;
        } else if (stopReason != ExecutionStopReason::None) {
            category = Debugger::DebugExceptionCategory::ResourceError;
        }
        if (services.debugger != nullptr &&
            services.debugger->exceptionSafepoint(*L, category, error.what(),
                                                  error.hasErrorObject() ? &error.getErrorObject() : nullptr) ==
                Debugger::DebugSafepointResult::TerminateExecution) [[unlikely]] {
            throw RuntimeError("debugger requested execution termination");
        }
        throw;
    } catch (const std::bad_alloc&) {
        if (services.debugger != nullptr &&
            services.debugger->exceptionSafepoint(*L, Debugger::DebugExceptionCategory::ResourceError,
                                                  "memory allocation failed") ==
                Debugger::DebugSafepointResult::TerminateExecution) [[unlikely]] {
            throw RuntimeError("debugger requested execution termination");
        }
        throw;
    } catch (const std::exception& error) {
        if (services.debugger != nullptr &&
            services.debugger->exceptionSafepoint(*L, Debugger::DebugExceptionCategory::RuntimeError,
                                                  error.what()) ==
                Debugger::DebugSafepointResult::TerminateExecution) [[unlikely]] {
            throw RuntimeError("debugger requested execution termination");
        }
        throw;
    } catch (...) {
        if (services.debugger != nullptr &&
            services.debugger->exceptionSafepoint(*L, Debugger::DebugExceptionCategory::RuntimeError,
                                                  "unknown runtime error") ==
                Debugger::DebugSafepointResult::TerminateExecution) [[unlikely]] {
            throw RuntimeError("debugger requested execution termination");
        }
        throw;
    }
}

std::expected<ExecResult, RuntimeError> tryExecuteProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    if (L == nullptr) {
        return std::unexpected(RuntimeError("VM::executeProto: null state"));
    }
    RuntimeServices services(L->getGlobalState());
    return tryExecuteProto(services, L, proto, nexeccalls);
}

std::expected<ExecResult, RuntimeError> tryExecuteProto(RuntimeServices& services, LuaState* L, Proto* proto,
                                                        i32 nexeccalls) {
    return VM::detail::captureRuntimeErrors<ExecResult>([&]() { return executeProto(services, L, proto, nexeccalls); });
}

namespace {

void enforceExecutionPolicy(GlobalState& globalState) {
    const ExecutionStopReason reason = globalState.getExecutionPolicy().consumeInstruction();
    if (reason != ExecutionStopReason::None) [[unlikely]] {
        throw RuntimeError(Value(globalState.getExecutionPolicyErrorMessage(reason)));
    }
}

enum class DispatchBackend : u8 {
    Switch,
    Table,
};

/**
 * @brief 调度时序说明
 *
 * runDispatchBackend 保持“下一程序计数器”不变量。取指后，`pc` 立即指向下一条指令；分支、
 * TEST 跳过、CALL 重入与表操作处理器都会调整该下一位置。任何调试、计数或行钩子运行前，
 * `CallInfo::savedpc` 保存同一个可恢复位置，而 `instructionPc` 仍指向当前指令，供行钩子与
 * 跟踪使用。新 Lua 调用帧尚无保存位置，因此空 savedpc 从 PC 0 开始。计数钩子先于行钩子
 * 运行；两者都可能修改栈或调用信息，所以执行操作码前会在每个钩子之后刷新 `base`。
 */
ExecResult runDispatchBackend(VMContext& context, DispatchBackend backend) {
    RuntimeServices& services = context.services;
    LuaState* L = context.state;
    Proto* proto = context.proto;
    i32 nexeccalls = context.nexeccalls;

    // 深度检查
    if (!proto)
        throw RuntimeError("VM::executeProto: null proto");
    if (nexeccalls >= MAX_CALLS)
        throw StackOverflowError("VM: stack overflow (too many nested calls)");

    // ---- 局部执行状态 ----
    Function* func = nullptr;
    Value* base = nullptr;
    usize pc = 0;
    Debugger::DebugController* const debugger = services.debugger;

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
    pc = ci.savedpc ? static_cast<usize>(ci.savedpc - entryCode.data()) : 0;

    /** @brief 在入口处转储字节码。 */
    if (VM::detail::shouldDumpBytecode(L)) {
        const auto dcode = proto->getInstructionSpan();
        std::fprintf(stderr, "[BCDUMP] proto(%p) %zu instructions, pc=%zu\n", static_cast<const void*>(proto),
                     dcode.size(), pc);
        for (usize di = 0; di < dcode.size(); di++) {
            Instruction dinst = dcode[di];
            std::fprintf(stderr, "  [%zu] op=%d A=%d B=%d C=%d Bx=%d sBx=%d\n", di, static_cast<int>(GET_OPCODE(dinst)),
                         GETARG_A(dinst), GETARG_B(dinst), GETARG_C(dinst), GETARG_Bx(dinst), GETARG_sBx(dinst));
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
            enforceExecutionPolicy(services.globalState);

            usize instructionPc = pc;
            Instruction inst = code[pc];
            OpCode op = GET_OPCODE(inst);
            pc++;

            CallInfo& currentCI = L->getCurrentCallInfo();
            currentCI.savedpc = code.data() + pc;

            if (debugger != nullptr && debugger->requiresInstructionSafepoint()) [[unlikely]] {
                const Debugger::DebugSafepointResult debugResult =
                    debugger->instructionSafepoint(*L, *proto, instructionPc);
                if (debugResult == Debugger::DebugSafepointResult::TerminateExecution) {
                    throw RuntimeError("debugger requested execution termination");
                }
            }

            VM::detail::dispatchCountHook(L);
            base = refreshBase(L);
            VM::detail::dispatchLineHook(L, proto, instructionPc);
            base = refreshBase(L);

            const bool traceDiff = VM::isTraceDiffEnabled(services) && VM::getTraceSink(services) != nullptr;
            const usize traceFrameBase = L->getCurrentCallInfo().base;
            const i32 traceCallDepth = nexeccalls;
            Vec<Value> traceBefore;
            if (traceDiff) {
                traceBefore = VM::detail::captureTraceRegisters(L, traceFrameBase, proto->getMaxStackSize());
            } else {
                VM::detail::emitInstructionTrace(L, proto, base, instructionPc, inst, nexeccalls);
            }

            if (backend == DispatchBackend::Table) {
                OpExecutionContext opContext{services, L, func, proto, base, pc, instructionPc, nexeccalls};
                HandlerStatus status = VM::runHandler(opContext, inst);
                base = opContext.base;
                nexeccalls = opContext.nexeccalls;
                if (traceDiff) {
                    VM::detail::emitInstructionTraceDiff(proto, L, traceFrameBase, instructionPc, inst, traceCallDepth,
                                                         traceBefore);
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

            OpExecutionContext opContext{services, L, func, proto, base, pc, instructionPc, nexeccalls};
            HandlerStatus status = HandlerStatus::Continue;

            switch (op) {

            case OpCode::MOVE:
                status = VM::detail::execOpMove(opContext, inst);
                break;
            case OpCode::LOADK:
                status = VM::detail::execOpLoadK(opContext, inst);
                break;
            case OpCode::LOADBOOL:
                status = VM::detail::execOpLoadBool(opContext, inst);
                break;
            case OpCode::LOADNIL:
                status = VM::detail::execOpLoadNil(opContext, inst);
                break;
            case OpCode::GETGLOBAL:
                status = VM::detail::execOpGetGlobal(opContext, inst);
                break;
            case OpCode::SETGLOBAL:
                status = VM::detail::execOpSetGlobal(opContext, inst);
                break;
            case OpCode::GETUPVAL:
                status = VM::detail::execOpGetUpval(opContext, inst);
                break;
            case OpCode::SETUPVAL:
                status = VM::detail::execOpSetUpval(opContext, inst);
                break;
            case OpCode::GETTABLE:
                status = VM::detail::execOpGetTable(opContext, inst);
                break;
            case OpCode::SETTABLE:
                status = VM::detail::execOpSetTable(opContext, inst);
                break;
            case OpCode::NEWTABLE:
                status = VM::detail::execOpNewTable(opContext, inst);
                break;
            case OpCode::SELF:
                status = VM::detail::execOpSelf(opContext, inst);
                break;
            case OpCode::SETLIST:
                status = VM::detail::execOpSetList(opContext, inst);
                break;
            case OpCode::ADD:
                status = VM::detail::execOpAdd(opContext, inst);
                break;
            case OpCode::SUB:
                status = VM::detail::execOpSub(opContext, inst);
                break;
            case OpCode::MUL:
                status = VM::detail::execOpMul(opContext, inst);
                break;
            case OpCode::DIV:
                status = VM::detail::execOpDiv(opContext, inst);
                break;
            case OpCode::MOD:
                status = VM::detail::execOpMod(opContext, inst);
                break;
            case OpCode::POW:
                status = VM::detail::execOpPow(opContext, inst);
                break;
            case OpCode::UNM:
                status = VM::detail::execOpUnm(opContext, inst);
                break;
            case OpCode::NOT:
                status = VM::detail::execOpNot(opContext, inst);
                break;
            case OpCode::LEN:
                status = VM::detail::execOpLen(opContext, inst);
                break;
            case OpCode::CONCAT:
                status = VM::detail::execOpConcat(opContext, inst);
                break;
            case OpCode::JMP:
                status = VM::detail::execOpJmp(opContext, inst);
                break;
            case OpCode::EQ:
                status = VM::detail::execOpEq(opContext, inst);
                break;
            case OpCode::LT:
                status = VM::detail::execOpLt(opContext, inst);
                break;
            case OpCode::LE:
                status = VM::detail::execOpLe(opContext, inst);
                break;
            case OpCode::TEST:
                status = VM::detail::execOpTest(opContext, inst);
                break;
            case OpCode::TESTSET:
                status = VM::detail::execOpTestSet(opContext, inst);
                break;
            case OpCode::CALL:
                status = VM::detail::execOpCall(opContext, inst);
                break;
            case OpCode::TAILCALL:
                status = VM::detail::execOpTailCall(opContext, inst);
                break;
            case OpCode::RETURN:
                status = VM::detail::execOpReturn(opContext, inst);
                break;
            case OpCode::CLOSE:
                status = VM::detail::execOpClose(opContext, inst);
                break;
            case OpCode::FORLOOP:
                status = VM::detail::execOpForLoop(opContext, inst);
                break;
            case OpCode::FORPREP:
                status = VM::detail::execOpForPrep(opContext, inst);
                break;
            case OpCode::TFORLOOP:
                status = VM::detail::execOpTForLoop(opContext, inst);
                break;
            case OpCode::CLOSURE:
                status = VM::detail::execOpClosure(opContext, inst);
                break;
            case OpCode::VARARG:
                status = VM::detail::execOpVararg(opContext, inst);
                break;

                // ============== 未知指令 ==============

            default:
                throw RuntimeError("VM: unsupported opcode: " + Str(getOpName(op)));

            } // switch

            base = opContext.base;
            nexeccalls = opContext.nexeccalls;
            if (traceDiff) {
                VM::detail::emitInstructionTraceDiff(proto, L, traceFrameBase, instructionPc, inst, traceCallDepth,
                                                     traceBefore);
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
        } // while 循环
    } // 代码引用作用域

    return ExecResult::Returned;
}

} // namespace

ExecResult SwitchDispatch::run(VMContext& context) {
    return runDispatchBackend(context, DispatchBackend::Switch);
}

ExecResult TableDispatch::run(VMContext& context) {
    return runDispatchBackend(context, DispatchBackend::Table);
}

} // namespace VM

} // namespace Lua
