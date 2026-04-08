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
#include "vm/global_state.hpp"
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

    i32 nresults = L->getTop();  // 所有参数即 yield 值
    L->setStatus(ThreadStatus::Yield);
    L->setYieldResults(nresults);
    return 0;  // C 函数返回 0 — vmPrecall 会检测 Yield 状态
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
        case CoroutineStatus::Suspended: statusStr = "suspended"; break;
        case CoroutineStatus::Running:   statusStr = "running";   break;
        case CoroutineStatus::Normal:    statusStr = "normal";    break;
        case CoroutineStatus::Dead:      statusStr = "dead";      break;
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
// coroutine.wrap(f) → function  (Phase 1 stub)
// =====================================================================

static i32 coroutine_wrap(LuaState* L) {
    // Phase 1: 暂不实现完整的 wrap
    L->error("coroutine.wrap is not yet implemented");
    return 0;
}

// =====================================================================
// 模块注册
// =====================================================================

void CoroutineLibModule::registerFunctions(LuaState* L) {
    if (!L) return;

    Table* coTable = FunctionRegistrar::createLibTable(L, "coroutine");
    if (!coTable) {
        L->error("Failed to create coroutine library table");
        return;
    }

    FunctionRegistrar(L)
        .addGlobal("create",  coroutine_create)
        .addGlobal("resume",  coroutine_resume)
        .addGlobal("yield",   coroutine_yield)
        .addGlobal("status",  coroutine_status)
        .addGlobal("running", coroutine_running)
        .addGlobal("wrap",    coroutine_wrap)
        .commitToTable(coTable);
}

void openCoroutineLib(LuaState* L) {
    if (!L) return;
    CoroutineLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
