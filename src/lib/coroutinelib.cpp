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
// coroutine.create(f) → 线程
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
/** @brief 恢复协程，返回成功标志及结果，或失败标志及错误。 */
// =====================================================================

static i32 coroutine_resume(LuaState* L) {
    i32 totalArgs = L->getTop();
    if (totalArgs < 1 || !L->at(1).isThread()) {
        L->error("bad argument #1 to 'resume' (coroutine expected)");
    }

    Thread* thread = L->at(1).asThread();
    i32 resumeArgs = totalArgs - 1;

    // Thread::resume 从 L 的栈顶传递恢复参数，再将 true/false 与结果压回 L
    thread->resume(L, resumeArgs);

    // 返回值数量等于 resume 压入的全部值（true/false 与结果）
    // L->getTop() 包含位置 1 的线程参数
    return L->getTop() - 1;
}

// =====================================================================
// coroutine.yield(...) → 在恢复方协程中返回值
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
// coroutine.status(co) → 字符串
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
// coroutine.running() → 线程或 nil
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
// coroutine.wrap(f) → 函数
//
// 创建协程并返回一个迭代器函数；每次调用该函数相当于 resume，
// 但直接返回 yield 值（不含前导 true），出错时直接抛出错误。
// =====================================================================

/**
 * @brief wrap 返回的迭代器函数；upvalue[0] 存储 Thread*
 */
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
    //   [... | 成功标志 | 结果1 | 结果2 | ...]
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
    L->consumeNativeWork(nresults == 0 ? 1 : static_cast<u64>(nresults));
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

/** @brief 创建 C 闭包，将协程对象作为关闭上值。 */
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
    L->requireStandardLibrary("coroutine");
    CoroutineLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
