/**
 * @file thread.cpp
 * @brief Lua Thread (协程) 实现
 */

#include "core/thread.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm.hpp"
#include "core/function.hpp"
#include "core/value.hpp"
#include "core/upvalue.hpp"
#include "core/gc_string.hpp"
#include "core/userdata.hpp"
#include "vm/global_state.hpp"
#include "gc/garbage_collector.hpp"
#include "common/lua_error.hpp"

namespace Lua {

// =====================================================================
// 构造/析构
// =====================================================================

Thread::Thread(LuaState* state)
    : GCObject(GCObjectType::Thread)
    , state_(state)
    , coStatus_(CoroutineStatus::Suspended)
{
    state_->setThread(this);
}

Thread::~Thread() {
    delete state_;
    state_ = nullptr;
}

// =====================================================================
// 工厂方法
// =====================================================================

Thread* Thread::create(LuaState* parentL, Function* func) {
    LuaState* coState = LuaState::newThread(parentL);

    // Stack: [nil(0), func(1)]
    coState->pushValue(Value(func));

    Thread* thread = new Thread(coState);
    parentL->getGlobalState().getGC().registerObject(thread);
    return thread;
}

// =====================================================================
// resume 核心逻辑
// =====================================================================

bool Thread::resume(LuaState* callerL, i32 nargs) {
    // ──── 前置检查 ────
    if (coStatus_ == CoroutineStatus::Dead) {
        callerL->pushBoolean(false);
        auto& pool = callerL->getGlobalState().getStringPool();
        callerL->pushString(pool.intern("cannot resume dead coroutine"));
        return false;
    }
    if (coStatus_ == CoroutineStatus::Running) {
        callerL->pushBoolean(false);
        auto& pool = callerL->getGlobalState().getStringPool();
        callerL->pushString(pool.intern("cannot resume running coroutine"));
        return false;
    }

    // ──── 值传递：callerL → coState ────
    if (nargs > 0) {
        Stack& srcStack = callerL->getStack();
        usize srcTop = callerL->getAbsoluteTop();
        usize start = srcTop - static_cast<usize>(nargs);
        for (usize i = start; i < srcTop; i++) {
            state_->pushValue(srcStack.at(i));
        }
        callerL->setAbsoluteTop(start);
    }

    // ──── 设置调用帧 / 调整 yield 返回值 ────
    Proto* proto = nullptr;

    if (firstResume_) {
        // 首次 resume：在协程栈上创建函数的调用帧
        Stack& stack = state_->getStack();
        usize funcPos = 1;
        Function* func = stack.at(funcPos).asFunction();
        proto = func->getProto();
        i32 numParams = proto->getNumParams();

        usize base;
        if (proto->isVararg()) {
            i32 actualArgs = nargs;
            usize oldBase = funcPos + 1;
            while (actualArgs < numParams) { stack.push(Value()); actualArgs++; }
            base = oldBase + static_cast<usize>(actualArgs);
            stack.checkSpace(static_cast<usize>(numParams) + 1);
            for (i32 i = 0; i < numParams; i++) {
                stack.push(stack[oldBase + i]);
                stack[oldBase + i] = Value();
            }
        } else {
            base = funcPos + 1;
            i32 actualArgs = nargs;
            while (actualArgs < numParams) { stack.push(Value()); actualArgs++; }
        }

        CallInfo& ci = state_->pushCallInfo();
        ci.func = funcPos;
        ci.base = base;
        ci.top = base + proto->getMaxStackSize();
        ci.nresults = MULTRET;
        ci.savedpc = nullptr;
        ci.tailcalls = 0;

        while (stack.size() < ci.top) stack.push(Value());
        state_->setAbsoluteTop(ci.top);

        if (state_->hasDebugHookMask(HookMaskCall)) {
            state_->callDebugHook(DebugHookEvent::Call);
        }

        firstResume_ = false;
        savedNexeccalls_ = 1;
    } else {
        // 后续 resume：resume 参数成为上次 yield 的返回值
        // yield C 函数的 CallInfo 仍在调用栈顶
        CallInfo& yieldCI = state_->getCurrentCallInfo();
        usize funcPos = yieldCI.func;
        i32 wantedResults = yieldCI.nresults;
        state_->popCallInfo();

        Stack& stack = state_->getStack();
        usize srcTop = state_->getAbsoluteTop();
        usize argStart = srcTop - static_cast<usize>(nargs);

        // 将 resume 参数放到 funcPos（模拟 vmPostcall）
        for (i32 i = 0; i < nargs; i++) {
            stack.at(funcPos + static_cast<usize>(i)) =
                stack.at(argStart + static_cast<usize>(i));
        }

        if (wantedResults >= 0) {
            for (i32 i = nargs; i < wantedResults; i++) {
                stack.at(funcPos + static_cast<usize>(i)) = Value();
            }
            state_->setAbsoluteTop(funcPos + static_cast<usize>(wantedResults));
        } else {
            // MULTRET
            state_->setAbsoluteTop(funcPos + static_cast<usize>(nargs));
        }

        // 获取当前 Lua 帧的 proto（reentry 会用到）
        // 恢复调用帧的栈窗口（模拟 CALL handler 的 post-processing）
        CallInfo& ci = state_->getCurrentCallInfo();
        state_->setAbsoluteTop(ci.top);
        Function* func = state_->getStack().at(ci.func).asFunction();
        proto = func->getProto();
    }

    // ──── 状态切换 ────
    Thread* callerThread = callerL->getThread();
    CoroutineStatus prevCallerStatus = CoroutineStatus::Dead;
    if (callerThread) {
        prevCallerStatus = callerThread->coStatus_;
        callerThread->coStatus_ = CoroutineStatus::Normal;
    }
    caller_ = callerThread;
    coStatus_ = CoroutineStatus::Running;
    state_->setStatus(ThreadStatus::OK);
    state_->incAllowYield();
    callerL->getGlobalState().setRunningThread(this);

    // ──── 调用 VM ────
    ExecResult result;
    try {
        result = VM::executeProto(state_, proto, savedNexeccalls_);
    } catch (const LuaError& e) {
        state_->decAllowYield();
        coStatus_ = CoroutineStatus::Dead;
        if (callerThread) callerThread->coStatus_ = prevCallerStatus;
        callerL->getGlobalState().setRunningThread(callerThread);
        callerL->pushBoolean(false);
        callerL->pushValue(e.getErrorObject());
        return false;
    } catch (const std::exception& e) {
        state_->decAllowYield();
        coStatus_ = CoroutineStatus::Dead;
        if (callerThread) callerThread->coStatus_ = prevCallerStatus;
        callerL->getGlobalState().setRunningThread(callerThread);
        callerL->pushBoolean(false);
        auto& pool = callerL->getGlobalState().getStringPool();
        callerL->pushString(pool.intern(e.what()));
        return false;
    }

    state_->decAllowYield();

    // ──── 处理结果 ────
    if (result == ExecResult::Yielded) {
        // yield：保存执行深度
        savedNexeccalls_ = state_->getSavedNexeccalls();
        coStatus_ = CoroutineStatus::Suspended;
        if (callerThread) callerThread->coStatus_ = prevCallerStatus;
        callerL->getGlobalState().setRunningThread(callerThread);

        // yield 值在 yield C 函数的 CallInfo 参数区
        callerL->pushBoolean(true);
        i32 nYieldResults = state_->getYieldResults();
        CallInfo& yieldCI = state_->getCurrentCallInfo();
        Stack& coStack = state_->getStack();
        for (i32 i = 0; i < nYieldResults; i++) {
            callerL->pushValue(coStack.at(yieldCI.base + static_cast<usize>(i)));
        }
        return true;
    } else {
        // 正常返回：协程结束
        coStatus_ = CoroutineStatus::Dead;
        if (callerThread) callerThread->coStatus_ = prevCallerStatus;
        callerL->getGlobalState().setRunningThread(callerThread);

        // 返回值由 RETURN 指令放在 ci.func 位置
        callerL->pushBoolean(true);
        CallInfo& ci = state_->getCurrentCallInfo();
        Stack& coStack = state_->getStack();
        usize funcPos = ci.func;
        usize top = state_->getAbsoluteTop();
        for (usize i = funcPos; i < top; i++) {
            callerL->pushValue(coStack.at(i));
        }
        return true;
    }
}

// =====================================================================
// GCObject 接口
// =====================================================================

void Thread::mark() {
    Stack& stack = state_->getStack();
    usize top = state_->getAbsoluteTop();

    // 标记协程栈上的所有 GC 对象
    for (usize i = 0; i < top && i < stack.size(); i++) {
        Value& v = stack.at(i);
        if (v.isCollectable()) {
            if (v.isString())       v.asString()->setColor(GCColor::Gray);
            else if (v.isTable())   v.asTable()->setColor(GCColor::Gray);
            else if (v.isFunction()) v.asFunction()->setColor(GCColor::Gray);
            else if (v.isUserdata()) v.asUserdata()->setColor(GCColor::Gray);
            else if (v.isThread())  v.asThread()->setColor(GCColor::Gray);
        }
    }

    // 标记 open upvalues
    Upvalue* uv = state_->getOpenUpvalues();
    while (uv) {
        uv->setColor(GCColor::Gray);
        uv = uv->getNext();
    }
}

usize Thread::getSize() const {
    return sizeof(Thread) + sizeof(LuaState)
        + state_->getStack().capacity() * sizeof(Value)
        + state_->getCallStack().capacity() * sizeof(CallInfo);
}

} // namespace Lua
