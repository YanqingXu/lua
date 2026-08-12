/**
 * @file thread.cpp
 * @brief Lua 协程实现
 */

#include "core/thread.hpp"
#include "common/features.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "vm/vm_internal.hpp"
#include "core/function.hpp"
#include "core/value.hpp"
#include "core/upvalue.hpp"
#include "core/gc_string.hpp"
#include "core/userdata.hpp"
#include "vm/state/global_state.hpp"
#include "runtime/runtime_services.hpp"
#include "gc/garbage_collector.hpp"
#include "common/lua_error.hpp"
#if LUA_CPP_ENABLE_DEBUGGER
#include "debugger/debug_runtime.hpp"
#endif
#include <algorithm>
#include <utility>

namespace Lua {

void LuaStateOwnerDeleter::operator()(LuaState* state) const noexcept {
    if (ownsState) {
        LuaState::destroyState(state);
    }
}

// =====================================================================
// 构造/析构
// =====================================================================

Thread::Thread(LuaStateOwner state)
    : GCObject(GCObjectType::Thread), state_(std::move(state)), coStatus_(CoroutineStatus::Suspended) {
    state_->setThread(this);
}

Thread::Thread(LuaState* mainState)
    : GCObject(GCObjectType::Thread), state_(mainState, LuaStateOwnerDeleter{false}),
      coStatus_(CoroutineStatus::Running) {
    if (mainState == nullptr) {
        throw std::invalid_argument("main thread facade requires a state");
    }
}

Thread::~Thread() = default;

// =====================================================================
// 工厂方法
// =====================================================================

Thread* Thread::create(LuaState* parentL, Function* func) {
    if (parentL == nullptr || func == nullptr) {
        throw std::invalid_argument("Thread::create requires parent state and function");
    }

    LuaStateOwner coState(LuaState::newThread(parentL));
    if (coState == nullptr) {
        throw std::bad_alloc();
    }

    /** @brief 栈布局：[空值(0)，函数(1)]。 */
    coState->pushValue(Value(func));

    return parentL->getGlobalState().getGC().create<Thread>(std::move(coState));
}

Thread* Thread::create(LuaState* parentL) {
    if (parentL == nullptr) {
        throw std::invalid_argument("Thread::create requires parent state");
    }

    LuaStateOwner coState(LuaState::newThread(parentL));
    if (coState == nullptr) {
        throw std::bad_alloc();
    }
    return parentL->getGlobalState().getGC().create<Thread>(std::move(coState));
}

// =====================================================================
/** @brief 协程恢复的核心逻辑。 */
// =====================================================================

bool Thread::resume(LuaState* callerL, i32 nargs) {
    if (callerL == nullptr) {
        return false;
    }

    const usize callerTop = callerL->getAbsoluteTop();
    const bool validArgumentCount = nargs >= 0 && static_cast<usize>(nargs) <= callerTop;
    const usize callerResultBase = validArgumentCount ? callerTop - static_cast<usize>(nargs) : callerTop;
    GlobalState& globalState = callerL->getGlobalState();
    Thread* callerThread = callerL->getThread();
    Thread* previousRunningThread = globalState.getRunningThread();
    const CoroutineStatus previousCallerStatus =
        callerThread != nullptr ? callerThread->coStatus_ : CoroutineStatus::Dead;
    bool callerStatusChanged = false;
    bool callerLinksChanged = false;
    bool yieldPermissionAdded = false;
    Stack& callerStack = callerL->getStack();

    auto clearCallerSlots = [&](usize begin, usize end) noexcept {
        const usize clearEnd = std::min(end, callerStack.size());
        for (usize slot = begin; slot < clearEnd; ++slot) {
            callerStack[slot] = Value();
        }
    };

    /**
     * @brief 在配置的 Lua 分配器拒绝全部请求时仍能发布失败响应。
     *
     * 正常情况下，调用者的调用帧已拥有这些栈槽，因此无需调用分配器即可写入。
     */
    auto publishPairNoThrow = [&](bool success, const Value& value) noexcept {
        callerL->setAbsoluteTop(callerResultBase);
        if (callerResultBase <= callerStack.size() && callerStack.size() - callerResultBase >= 2) {
            callerStack[callerResultBase] = Value(success);
            callerStack[callerResultBase + 1] = value;
            const usize publishedTop = callerResultBase + 2;
            clearCallerSlots(publishedTop, callerTop);
            callerL->setAbsoluteTop(publishedTop);
            return true;
        }

        const usize previousStackTop = callerStack.size();
        try {
            callerL->pushBoolean(success);
            callerL->pushValue(value);
        } catch (...) {
            try {
                callerStack.setTop(previousStackTop);
            } catch (...) {
                /** @brief 缩小到已保存的物理栈顶无需分配内存。 */
            }
            clearCallerSlots(callerResultBase, std::max(callerTop, callerResultBase + 2));
            callerL->setAbsoluteTop(callerResultBase);
            return false;
        }

        const usize publishedTop = callerResultBase + 2;
        clearCallerSlots(publishedTop, callerTop);
        callerL->setAbsoluteTop(publishedTop);
        return true;
    };

    auto restoreCallerContext = [&]() noexcept {
        if (yieldPermissionAdded) {
            state_->decAllowYield();
            yieldPermissionAdded = false;
        }
        if (callerStatusChanged && callerThread != nullptr) {
            callerThread->coStatus_ = previousCallerStatus;
            callerStatusChanged = false;
        }
        globalState.setRunningThread(previousRunningThread);
        if (callerLinksChanged) {
            caller_ = nullptr;
            callerState_ = nullptr;
            callerLinksChanged = false;
        }
    };

    auto failResume = [&](const Value& errorValue, ThreadStatus status, bool canonicalizeCoroutine) noexcept {
        restoreCallerContext();
        if (canonicalizeCoroutine) {
            abortResume(status);
        } else {
            state_->setStatus(status);
        }

        if (!publishPairNoThrow(false, errorValue)) {
            /**
             * @brief 调用者的两个预留结果槽均不可用时，无法表示 Lua 结果。
             *
             * 即便如此，也要保持每项运行时状态规范，并在边界内消化 C++ 分配失败。
             */
            callerL->setAbsoluteTop(callerResultBase);
        }
        return false;
    };

    auto failMemory = [&]() noexcept {
        return failResume(Value(globalState.getMemoryErrorMessage()), ThreadStatus::ErrMem, true);
    };

    auto failRuntimeValue = [&](const Value& errorValue, bool canonicalizeCoroutine) noexcept {
        if (canonicalizeCoroutine) {
            return failResume(errorValue, ThreadStatus::ErrRun, true);
        }

        restoreCallerContext();
        state_->setStatus(ThreadStatus::ErrRun);
        coStatus_ = CoroutineStatus::Dead;
        if (!publishPairNoThrow(false, errorValue)) {
            callerL->setAbsoluteTop(callerResultBase);
        }
        return false;
    };

    auto failRuntimeMessage = [&](CharPtr message, bool canonicalizeCoroutine) noexcept {
        try {
            return failRuntimeValue(Value(globalState.getStringPool().intern(message)), canonicalizeCoroutine);
        } catch (...) {
            return failRuntimeValue(Value(globalState.getApiExceptionMessage()), canonicalizeCoroutine);
        }
    };

    auto failMessage = [&](CharPtr message, bool canonicalizeCoroutine) noexcept {
        try {
            Value errorValue(globalState.getStringPool().intern(message));
            return failResume(errorValue, ThreadStatus::ErrRun, canonicalizeCoroutine);
        } catch (...) {
            return failResume(Value(globalState.getApiExceptionMessage()), ThreadStatus::ErrRun, canonicalizeCoroutine);
        }
    };

    if (!validArgumentCount) {
        return failMessage("invalid resume argument count", false);
    }
    if (coStatus_ == CoroutineStatus::Dead) {
        return failMessage("cannot resume dead coroutine", false);
    }
    if (coStatus_ == CoroutineStatus::Running) {
        return failMessage("cannot resume running coroutine", false);
    }
    if (firstResume_) {
        Stack& stack = state_->getStack();
        if (state_->getAbsoluteTop() <= 1 || !stack.at(1).isFunction()) {
            return failMessage("cannot resume coroutine without an entry function", true);
        }
        Function* entry = stack.at(1).asFunction();
        if (entry == nullptr || entry->isCFunction()) {
            return failMessage("cannot resume a C function as coroutine entry", true);
        }
    }

    try {
        /**
         * @brief 将参数从 callerL 复制到协程。
         *
         * 所有复制均成功前不要消费调用者参数，以便失败路径拥有稳定的基准位置。
         */
        if (nargs > 0) {
            Stack& sourceStack = callerL->getStack();
            for (usize i = callerResultBase; i < callerTop; ++i) {
                state_->pushValue(sourceStack.at(i));
            }
        }
        clearCallerSlots(callerResultBase, callerTop);
        callerL->setAbsoluteTop(callerResultBase);

        Proto* proto = nullptr;
        if (firstResume_) {
            Stack& stack = state_->getStack();
            constexpr usize functionPosition = 1;
            Function* function = stack.at(functionPosition).asFunction();
            proto = function->getProto();
            if (proto == nullptr) {
                throw RuntimeError("coroutine entry has no prototype");
            }
            const i32 parameterCount = proto->getNumParams();

            usize base = functionPosition + 1;
            if (proto->isVararg()) {
                i32 actualArguments = nargs;
                const usize oldBase = functionPosition + 1;
                while (actualArguments < parameterCount) {
                    stack.push(Value());
                    ++actualArguments;
                }
                base = oldBase + static_cast<usize>(actualArguments);
                stack.checkSpace(static_cast<usize>(parameterCount) + 1);
                for (i32 i = 0; i < parameterCount; ++i) {
                    stack.push(stack[oldBase + static_cast<usize>(i)]);
                    stack[oldBase + static_cast<usize>(i)] = Value();
                }
            } else {
                i32 actualArguments = nargs;
                while (actualArguments < parameterCount) {
                    stack.push(Value());
                    ++actualArguments;
                }
            }

            CallInfo& callInfo = state_->pushCallInfo();
            callInfo.func = functionPosition;
            callInfo.base = base;
            callInfo.top = base + proto->getMaxStackSize();
            callInfo.nresults = MULTRET;
            callInfo.savedpc = nullptr;
            callInfo.tailcalls = 0;

            while (stack.size() < callInfo.top) {
                stack.push(Value());
            }
            const usize registerClearEnd =
                proto->isVararg() ? callInfo.top : std::max(callInfo.top, base + static_cast<usize>(nargs));
            for (usize slot = base + static_cast<usize>(parameterCount); slot < registerClearEnd; ++slot) {
                stack[slot] = Value();
            }
            if (stack.size() > callInfo.top) {
                stack.setTop(callInfo.top);
            }
            state_->setAbsoluteTop(callInfo.top);

            VM::detail::dispatchCallHook(state_.get());

            firstResume_ = false;
            savedNexeccalls_ = 1;
        } else {
            /** @brief 恢复参数替换暂停的挂起调用结果。 */
            CallInfo& yieldCallInfo = state_->getCurrentCallInfo();
            const usize functionPosition = yieldCallInfo.func;
            const i32 wantedResults = yieldCallInfo.nresults;
            state_->popCallInfo();

            Stack& stack = state_->getStack();
            const usize sourceTop = state_->getAbsoluteTop();
            const usize argumentStart = sourceTop - static_cast<usize>(nargs);
            const usize argumentCount = static_cast<usize>(nargs);
            if (functionPosition > argumentStart && functionPosition < sourceTop) {
                for (usize i = argumentCount; i > 0; --i) {
                    stack.at(functionPosition + i - 1) = stack.at(argumentStart + i - 1);
                }
            } else {
                for (usize i = 0; i < argumentCount; ++i) {
                    stack.at(functionPosition + i) = stack.at(argumentStart + i);
                }
            }

            const bool fixedResults = wantedResults >= 0;
            if (fixedResults) {
                for (usize i = argumentCount; i < static_cast<usize>(wantedResults); ++i) {
                    stack.at(functionPosition + i) = Value();
                }
            }

            const usize logicalResultTop =
                functionPosition + (fixedResults ? static_cast<usize>(wantedResults) : argumentCount);
            const usize copiedResultTop = functionPosition + argumentCount;
            for (usize slot = logicalResultTop; slot < copiedResultTop; ++slot) {
                stack[slot] = Value();
            }
            for (usize slot = argumentStart; slot < sourceTop; ++slot) {
                if (slot < functionPosition || slot >= logicalResultTop) {
                    stack[slot] = Value();
                }
            }
            state_->setAbsoluteTop(logicalResultTop);

            CallInfo& callInfo = state_->getCurrentCallInfo();
            const usize physicalTop = std::max(callInfo.top, logicalResultTop);
            if (stack.size() > physicalTop) {
                stack.setTop(physicalTop);
            }
            if (fixedResults) {
                state_->setAbsoluteTop(callInfo.top);
            }
            Function* function = state_->getStack().at(callInfo.func).asFunction();
            proto = function != nullptr ? function->getProto() : nullptr;
            if (proto == nullptr) {
                throw RuntimeError("suspended coroutine has no Lua frame");
            }
        }

        if (callerThread != nullptr) {
            callerThread->coStatus_ = CoroutineStatus::Normal;
            callerStatusChanged = true;
        }
        caller_ = callerThread;
        callerState_ = callerL;
        callerLinksChanged = true;
        coStatus_ = CoroutineStatus::Running;
        state_->setStatus(ThreadStatus::OK);
        state_->incAllowYield();
        yieldPermissionAdded = true;
        globalState.setRunningThread(this);

#if LUA_CPP_ENABLE_DEBUGGER
        if (Debugger::DebugController* debugger = globalState.getDebugController();
            debugger != nullptr &&
            debugger->semanticSafepoint(*state_, Debugger::DebugSemanticEvent::CoroutineResume) ==
                Debugger::DebugSafepointResult::TerminateExecution) [[unlikely]] {
            throw RuntimeError("debugger requested execution termination");
        }
#endif

        RuntimeServices services(state_->getGlobalState());
        const ExecResult result = VM::executeProto(services, state_.get(), proto, savedNexeccalls_);
        restoreCallerContext();

        Stack& coroutineStack = state_->getStack();
        usize resultStart = 0;
        usize resultCount = 0;
        if (result == ExecResult::Yielded) {
#if LUA_CPP_ENABLE_DEBUGGER
            if (Debugger::DebugController* debugger = globalState.getDebugController();
                debugger != nullptr &&
                debugger->semanticSafepoint(*state_, Debugger::DebugSemanticEvent::CoroutineYield) ==
                    Debugger::DebugSafepointResult::TerminateExecution) [[unlikely]] {
                throw RuntimeError("debugger requested execution termination");
            }
#endif
            savedNexeccalls_ = state_->getSavedNexeccalls();
            coStatus_ = CoroutineStatus::Suspended;
            const i32 yieldResults = state_->getYieldResults();
            if (yieldResults < 0 || static_cast<usize>(yieldResults) > state_->getAbsoluteTop()) {
                throw RuntimeError("coroutine yielded an invalid result count");
            }
            resultCount = static_cast<usize>(yieldResults);
            resultStart = state_->getAbsoluteTop() - resultCount;
        } else {
            state_->setStatus(ThreadStatus::OK);
            coStatus_ = CoroutineStatus::Dead;
            const CallInfo& callInfo = state_->getCurrentCallInfo();
            resultStart = callInfo.func;
            const usize resultTop = state_->getAbsoluteTop();
            if (resultStart > resultTop) {
                throw RuntimeError("coroutine returned an invalid result range");
            }
            resultCount = resultTop - resultStart;
        }

        callerL->setAbsoluteTop(callerResultBase);
        callerL->pushBoolean(true);
        for (usize i = 0; i < resultCount; ++i) {
            callerL->pushValue(coroutineStack.at(resultStart + i));
        }
        const usize publishedTop = callerL->getAbsoluteTop();
        clearCallerSlots(publishedTop, callerTop);
        for (usize i = 0; i < resultCount; ++i) {
            coroutineStack[resultStart + i] = Value();
        }
        return true;
    } catch (const MemoryError&) {
        return failMemory();
    } catch (const std::bad_alloc&) {
        return failMemory();
    } catch (const LuaError& error) {
        if (error.hasErrorObject()) {
            /**
             * @brief Lua 5.1 在恢复报告错误后仍保留失败协程的调用帧，供调试回溯函数使用。
             */
            return failRuntimeValue(error.getErrorObject(), false);
        }
        return failRuntimeMessage(error.what(), false);
    } catch (const std::exception& error) {
        /**
         * @brief 外部异常可在任意位置中断调用帧修改，因此在边界内消化异常时使用更强的回滚。
         */
        return failRuntimeMessage(error.what(), true);
    } catch (...) {
        return failRuntimeValue(Value(globalState.getApiExceptionMessage()), true);
    }
}

void Thread::abortResume(ThreadStatus status) noexcept {
    while (state_->getCurrentCI() > 0) {
        const usize frameBase = state_->getCurrentCallInfo().base;
        try {
            state_->closeUpvalues(frameBase);
        } catch (...) {
            /** @brief 关闭失败时不得暴露构造不完整的调用信息。 */
        }
        try {
            state_->popCallInfo();
        } catch (...) {
            /**
             * @brief popCallInfo 仅拒绝基础调用帧；若未来实现增加其他抛出操作，则防御性停止。
             */
            break;
        }
    }

    Stack& stack = state_->getStack();
    for (usize slot = 0; slot < stack.capacity(); ++slot) {
        stack[slot] = Value();
    }
    stack.clear();
    state_->setAbsoluteTop(0);
    state_->setYieldResults(0);
    state_->setSavedNexeccalls(1);
    state_->setStatus(status);
    coStatus_ = CoroutineStatus::Dead;
    firstResume_ = false;
    savedNexeccalls_ = 1;

    if (state_->getGlobalState().getRunningThread() == this) {
        state_->getGlobalState().setRunningThread(caller_);
    }
    if (caller_ != nullptr && caller_->coStatus_ == CoroutineStatus::Normal) {
        caller_->coStatus_ = CoroutineStatus::Running;
    }
    caller_ = nullptr;
    callerState_ = nullptr;
}

// =====================================================================
// GCObject 接口
// =====================================================================

void Thread::mark(GarbageCollector& gc) {
    gc.markState(state_.get());
    gc.markState(callerState_);
    gc.markObject(caller_);
}

usize Thread::getSize() const {
    if (!ownsLuaState()) {
        return sizeof(Thread);
    }
    return sizeof(Thread) + sizeof(LuaState) + state_->getStack().capacity() * sizeof(Value) +
           state_->getCallStack().capacity() * sizeof(CallInfo);
}

} // namespace Lua
