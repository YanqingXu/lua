/**
 * @file coroutinelib.cpp
 * @brief Lua协程库实现
 *
 * 实现 coroutine.create / resume / yield / status / running / wrap
 */

#include "lib/coroutinelib.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "core/thread.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "vm/state/global_state.hpp"
#include "common/lua_error.hpp"

namespace Lua {

// =====================================================================
// coroutine.create(f) → thread
// =====================================================================

static i32 coroutine_create(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isFunction()) {
        L->error("bad argument #1 to 'create' (function expected)");
    }

    Function* func = L->at(1).asFunction();
    Thread* thread = Thread::create(L, func);

    L->pushValue(Value(thread));
    return 1;
}

// =====================================================================
// coroutine.resume(co, ...) → true, ... | false, err
// =====================================================================

static i32 coroutine_resume(LuaState* L) {
    i32 totalArgs = L->getTop();
    if (totalArgs < 1 || !L->at(1).isThread()) {
        L->error("bad argument #1 to 'resume' (coroutine expected)");
    }

    Thread* thread = L->at(1).asThread();
    i32 resumeArgs = totalArgs - 1;

    // Thread::resume transfers resumeArgs from L's stack top,
    // then pushes true/false + results back to L
    thread->resume(L, resumeArgs);

    // Return count = everything pushed by resume (true/false + results)
    // L->getTop() includes the thread arg at position 1
    return L->getTop() - 1;
}

// =====================================================================
// coroutine.yield(...) → (returns values in resuming coroutine)
// =====================================================================

static i32 coroutine_yield(LuaState* L) {
    if (!L->canYield()) {
        L->error("cannot yield across non-resumable call boundaries");
    }

    i32 nresults = L->getTop(); // 所有参数即 yield 值
    L->setStatus(ThreadStatus::Yield);
    L->setYieldResults(nresults);
    return 0; // C 函数返回 0 — vmPrecall 会检测 Yield 状态
}

// =====================================================================
// coroutine.status(co) → string
// =====================================================================

static i32 coroutine_status(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isThread()) {
        L->error("bad argument #1 to 'status' (coroutine expected)");
    }

    Thread* thread = L->at(1).asThread();
    const char* statusStr = nullptr;

    switch (thread->getCoroutineStatus()) {
    case CoroutineStatus::Suspended:
        statusStr = "suspended";
        break;
    case CoroutineStatus::Running:
        statusStr = "running";
        break;
    case CoroutineStatus::Normal:
        statusStr = "normal";
        break;
    case CoroutineStatus::Dead:
        statusStr = "dead";
        break;
    }

    auto& pool = L->getGlobalState().getStringPool();
    L->pushString(pool.intern(statusStr));
    return 1;
}

// =====================================================================
// coroutine.running() → thread | nil
// =====================================================================

static i32 coroutine_running(LuaState* L) {
    Thread* running = L->getGlobalState().getRunningThread();
    if (running) {
        L->pushValue(Value(running));
    } else {
        L->pushNil();
    }
    return 1;
}

// =====================================================================
// coroutine.wrap(f) → function
//
// 创建协程并返回一个迭代器函数；每次调用该函数相当于 resume，
// 但直接返回 yield 值（不含前导 true），出错时直接抛出错误。
// =====================================================================

/// wrap 返回的迭代器函数；upvalue[0] 存储 Thread*
static i32 wrap_iterator(LuaState* L) {
    // 取出 upvalue 中的 Thread
    const CallInfo& ci = L->getCurrentCallInfo();
    Function* self = L->getStack()[ci.func].asFunction();
    Upvalue* uv = self->getUpvalue(0);
    if (!uv) {
        L->error("cannot resume dead coroutine");
    }
    Value threadVal = uv->getValue(L->getStack());
    if (!threadVal.isThread()) {
        L->error("cannot resume dead coroutine");
    }
    Thread* thread = threadVal.asThread();

    // 收集调用参数
    i32 nargs = L->getTop();

    // resume 会向 L 推入 true/false + 结果
    // 先记住调用前栈顶位置（参数区之后）
    usize beforeTop = L->getAbsoluteTop();

    // 将参数复制到 beforeTop 之后（resume 需要从栈顶取）
    // 参数已经在 base+1 .. base+nargs，直接 resume 即可
    thread->resume(L, nargs);

    // resume 替换了参数区，现在 L 栈顶布局：
    //   [... | bool_ok | result1 | result2 | ...]
    // getTop() 包含了原始 thread arg（这里没有），直接算
    usize afterTop = L->getAbsoluteTop();
    usize pushed = afterTop - beforeTop + static_cast<usize>(nargs);

    // 第一个推入的值是 bool（ok/fail）
    // 在 beforeTop - nargs 位置… 不对——resume 在推入时已清理了参数
    // 最简单：从 afterTop 回退 pushed 个位置取 bool
    usize resultBase = afterTop - pushed;
    Value okVal = L->getStack().at(resultBase);
    bool ok = okVal.isBoolean() && okVal.asBoolean();

    if (!ok) {
        // 错误：第二个值是错误消息，直接 error
        Value errMsg;
        if (pushed >= 2) {
            errMsg = L->getStack().at(resultBase + 1);
        }
        // 抛出 Lua 错误
        L->pushValue(errMsg);
        return L->error();
    }

    // 成功：把 bool 之后的结果值搬到栈帧起点
    i32 nresults = static_cast<i32>(pushed) - 1; // 减去 bool
    usize dst = resultBase;
    for (i32 i = 0; i < nresults; i++) {
        L->getStack().at(dst + static_cast<usize>(i)) = L->getStack().at(resultBase + 1 + static_cast<usize>(i));
    }
    const usize resultTop = dst + static_cast<usize>(nresults);
    for (usize slot = resultTop; slot < afterTop; ++slot) {
        L->getStack()[slot] = Value();
    }
    L->setAbsoluteTop(resultTop);

    return nresults;
}

static i32 coroutine_wrap(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isFunction()) {
        L->error("bad argument #1 to 'wrap' (function expected)");
    }

    Function* func = L->at(1).asFunction();
    Thread* thread = Thread::create(L, func);

    // 创建 C 闭包，将 thread 作为 closed upvalue
    Function* closure = L->getGlobalState().getGC().create<Function>(wrap_iterator);

    Upvalue* uv = L->getGlobalState().getGC().create<Upvalue>(Value(thread));
    closure->addUpvalue(uv);

    L->pushFunction(closure);
    return 1;
}

// =====================================================================
// 模块注册
// =====================================================================

void CoroutineLibModule::registerFunctions(LuaState* L) {
    if (!L)
        return;

    Table* coTable = FunctionRegistrar::createLibTable(L, "coroutine");
    if (!coTable) {
        L->error("Failed to create coroutine library table");
        return;
    }

    FunctionRegistrar(L)
        .addGlobal("create", coroutine_create)
        .addGlobal("resume", coroutine_resume)
        .addGlobal("yield", coroutine_yield)
        .addGlobal("status", coroutine_status)
        .addGlobal("running", coroutine_running)
        .addGlobal("wrap", coroutine_wrap)
        .commitToTable(coTable);
}

void openCoroutineLib(LuaState* L) {
    if (!L)
        return;
    CoroutineLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
