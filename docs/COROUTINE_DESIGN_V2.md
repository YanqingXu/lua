# Lua 协程实现技术方案 v2：显式 VM 状态机 + C++20 协程调度壳

> **基于 COROUTINE_DESIGN_ANALYSIS.MD 评审意见的完整重新设计**

---

## 0. 设计原则

本方案遵循评审文档的核心判断：

1. **Lua coroutine 的语义状态，必须以 VM 数据结构为真；C++ coroutine 只是外层调度壳**
2. **VM 使用显式返回码（`ExecResult::Yielded`）而非异常来表达 yield**
3. **先不引入 C++20 协程，先把 VM 层面的 yield/resume 状态机做通；C++20 协程留给第二阶段作为宿主调度层**
4. **所有可恢复状态必须落进 `LuaState` / `CallInfo`，不依赖 C++ 栈帧临时变量**

---

## 1. 与 v1 方案的关键差异

| 维度 | v1（YieldSignal 异常） | v2（显式 VM 状态机） |
|------|----------------------|---------------------|
| yield 通道 | `throw YieldSignal` | `return ExecResult::Yielded` |
| 执行现场保存 | 部分依赖 C++ coroutine frame | **100% 落在 LuaState/CallInfo** |
| C++20 coroutine | Phase 1 就引入 | **Phase 1 不引入**，Phase 3 作为宿主调度壳 |
| VM 恢复机制 | `VM::resumeExecution()` 新入口 | **现有 `executeProto` 的 `reentry` 标签天然支持** |
| 错误模型 | 依赖 `std::exception` | **统一 `LuaError` 类型** |
| yield 合法性 | 未编码 | **显式 `allowYield` 计数器** |
| resume chain | 仅 `callerState_` | **显式双向链表（caller ↔ callee）** |

---

## 2. 核心发现：现有 VM 已天然适合 yield/resume

分析 [vm.cpp](../src/vm/vm.cpp) 后，发现现有架构已具备关键条件：

### 2.1 reentry 机制

```cpp
void executeProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    Function* func = nullptr;
    Value*    base = nullptr;
    usize     pc   = 0;

reentry:  // ⭐ 从 CallInfo 恢复所有执行状态
    {
        CallInfo& ci = L->getCurrentCallInfo();
        func = stack[ci.func].asFunction();
        proto = func->getProto();
        pc = ci.savedpc ? (ci.savedpc - proto->getCode().data()) : 0;
        base = &stack[ci.base];
    }
    // ... 主执行循环 ...
}
```

**关键**：`reentry` 标签已经实现了"从 CallInfo 恢复完整执行状态"的逻辑——`func`、`proto`、`pc`、`base` 全部从 `LuaState` + `CallInfo` 重建。这意味着**只要 yield 时正确保存 `savedpc`，恢复时 `goto reentry` 就能正确续行**。

### 2.2 执行状态局部变量分析

`executeProto` 中的关键局部变量：

| 变量 | 来源 | yield 时需要保存？ |
|------|------|-------------------|
| `func` | `CallInfo.func` → `stack[ci.func]` | ❌ reentry 自动恢复 |
| `proto` | `func->getProto()` | ❌ reentry 自动恢复 |
| `pc` | `CallInfo.savedpc` | ✅ **yield 前必须写回** |
| `base` | `&stack[ci.base]` | ❌ reentry 自动恢复 |
| `nexeccalls` | 参数 | ✅ **需要保存到 Thread** |
| `code` (引用) | `proto->getCode()` | ❌ 从 proto 推导 |

结论：**只需额外保存 `savedpc` 和 `nexeccalls`**，其余所有状态 reentry 标签已负责重建。

### 2.3 CALL 指令已保存 savedpc

```cpp
case OpCode::CALL: {
    L->getCurrentCallInfo().savedpc = &code[pc];  // ⭐ 已保存
    bool isLua = vmPrecall(L, a, nArgs, nResults);
    if (isLua) { nexeccalls++; goto reentry; }
    // C 函数返回后...
    base = refreshBase(L);
}
```

`savedpc` 在 CALL 之前已保存，所以如果 C 函数（如 `coroutine.yield`）设置了 Yield 状态，此时 `savedpc` 已经正确指向 CALL 之后的下一条指令。

---

## 3. 类型定义

### 3.1 ExecResult：VM 执行结果

```cpp
// === vm.hpp 新增 ===

/// VM 执行结果（替代 void 返回，让 yield 成为正常控制流）
enum class ExecResult : u8 {
    Returned,      // 函数正常返回
    Yielded        // 协程 yield
};
```

### 3.2 LuaError：统一错误载体

```cpp
// === src/common/lua_error.hpp 新增 ===

#pragma once
#include "common/types.hpp"
#include "core/value.hpp"

namespace Lua {

/// 统一的 Lua 运行时错误
/// 
/// 替代当前散落的 std::runtime_error，成为 VM 内部唯一的错误抛出类型。
/// 错误对象本身就是一个 Lua Value（可以是 string，也可以是 table 或任意类型），
/// 与 Lua 5.1 的 error(msg) 语义对齐。
class LuaError : public std::exception {
public:
    explicit LuaError(Value errorObj)
        : errorObj_(std::move(errorObj)) {}

    explicit LuaError(const char* msg);  // 便捷构造：自动 intern 为 GCString

    const char* what() const noexcept override;
    const Value& getErrorObject() const noexcept { return errorObj_; }

private:
    Value errorObj_;
    mutable Str cachedWhat_;  // what() 的缓存
};

} // namespace Lua
```

### 3.3 CoroutineStatus：协程状态枚举

```cpp
// === src/core/thread.hpp ===

/// Lua 协程状态（与 ThreadStatus 不同，这是 Lua 层面语义）
enum class CoroutineStatus : u8 {
    Suspended,  // 创建后 / yield 后
    Running,    // 正在执行
    Normal,     // resume 了其他协程，自身暂停
    Dead        // 函数返回或出错
};
```

### 3.4 Thread 类

```cpp
// === src/core/thread.hpp ===

#pragma once
#include "core/gc_object.hpp"

namespace Lua {

class LuaState;
class Function;
class GarbageCollector;

enum class CoroutineStatus : u8 {
    Suspended,
    Running,
    Normal,
    Dead
};

/// Thread: GC 管理的协程对象
///
/// 核心设计：
///   - 每个 Thread 持有一个独立的 LuaState（独立栈 + 调用栈，共享 GlobalState）
///   - 所有执行现场保存在 LuaState/CallInfo 中（不依赖 C++ 栈帧）
///   - VM 通过 ExecResult::Yielded 退出执行循环
///   - resume/yield 通过显式的值搬运 + VM 重入实现
///
class Thread : public GCObject {
public:
    // === 工厂方法 ===

    /// 创建协程
    /// @param parentL 创建者的 LuaState（用于共享 GlobalState）
    /// @param func 协程要执行的 Lua 函数
    static Thread* create(LuaState* parentL, Function* func);

    ~Thread();

    // === 核心操作 ===

    /// resume 协程
    ///
    /// 语义：
    ///   1. 将 callerL 栈顶的 nargs 个值搬到协程栈
    ///   2. 首次 resume：参数作为协程函数的参数
    ///      后续 resume：参数作为上次 yield 的返回值
    ///   3. 调用 VM::executeProto 恢复执行
    ///   4. 若 VM 返回 Yielded：将 yield 值搬回 callerL 栈
    ///      若 VM 返回 Returned：将返回值搬回 callerL 栈
    ///      若异常：捕获错误，返回 false + errmsg
    ///
    /// @return true=成功（正常返回或 yield），false=错误
    bool resume(LuaState* callerL, i32 nargs);

    // === 状态查询 ===

    CoroutineStatus getCoroutineStatus() const noexcept { return coStatus_; }
    LuaState* getLuaState() const noexcept { return state_; }
    bool isDead() const noexcept { return coStatus_ == CoroutineStatus::Dead; }
    bool isSuspended() const noexcept { return coStatus_ == CoroutineStatus::Suspended; }

    // === resume 链管理 ===

    /// 获取恢复本协程的调用者线程
    Thread* getCaller() const noexcept { return caller_; }
    void setCaller(Thread* t) noexcept { caller_ = t; }

    // === GCObject 接口 ===

    void markChildren(GarbageCollector& gc) override;
    usize getMemorySize() const override;
    void destroy() override;

private:
    explicit Thread(LuaState* state);

    LuaState*       state_;      // 协程独立的 LuaState
    CoroutineStatus coStatus_;   // Lua 层协程状态

    /// resume 链：指向"是谁 resume 了我"的 Thread
    /// 主线程 resume 协程时为 nullptr（主线程没有 Thread 对象）
    Thread* caller_ = nullptr;

    /// 是否为首次 resume（决定参数放置方式）
    bool firstResume_ = true;

    /// yield 返回值数量（yield 时由 VM 写入）
    i32 yieldResults_ = 0;

    /// 执行深度（保存 nexeccalls，yield 时写入，resume 时恢复）
    i32 savedNexeccalls_ = 1;
};

} // namespace Lua
```

---

## 4. VM 层改造

### 4.1 executeProto 签名变更

```diff
 // vm.hpp
 namespace VM {
-    void executeProto(LuaState* L, Proto* proto, i32 nexeccalls = 1);
+    ExecResult executeProto(LuaState* L, Proto* proto, i32 nexeccalls = 1);
 }
```

**影响面分析**：`executeProto` 的调用点：
- `VM::execute()` — 最外层入口，检查返回值（主线程不应 yield）
- `CALL` → `goto reentry` 路径 — 不经过 return，无影响
- `RETURN` 中 `nexeccalls == 0` 时 — 改为 `return ExecResult::Returned`
- `LuaState::pcall()` — 检查 Yielded（pcall 中不允许 yield，直接报错）

### 4.2 CALL 指令增加 yield 检测

```cpp
case OpCode::CALL: {
    i32 nArgs    = b - 1;
    i32 nResults = c - 1;

    L->getCurrentCallInfo().savedpc = &code[pc];

    bool isLua = vmPrecall(L, a, nArgs, nResults);

    if (isLua) {
        nexeccalls++;
        goto reentry;
    }

    // C 函数已执行完毕 —— 检查是否触发了 yield
    if (L->getStatus() == ThreadStatus::Yield) {
        // ⭐ yield 检测：C 函数（coroutine.yield）设置了 Yield 状态
        // savedpc 已在上面保存，指向 CALL 之后的下一条指令
        // 直接返回，让 Thread::resume 处理后续
        return ExecResult::Yielded;
    }

    // 正常路径：恢复寄存器窗口
    {
        CallInfo& callerCI = L->getCurrentCallInfo();
        Stack& stack = L->getStack();
        stack.setTop(callerCI.top);
        L->setAbsoluteTop(callerCI.top);
    }
    base = refreshBase(L);
    break;
}
```

### 4.3 RETURN 指令变更

```cpp
case OpCode::RETURN: {
    // ... 计算返回值、移动值到 funcPos、关闭 upvalues ...

    if (--nexeccalls == 0) {
        return ExecResult::Returned;  // ⭐ 改为返回枚举
    }

    // 弹出 CallInfo，处理返回值
    {
        i32 funcPos       = static_cast<i32>(ci.func);
        i32 wantedResults = ci.nresults;
        L->popCallInfo();
        vmPostcall(L, funcPos, wantedResults);
    }
    goto reentry;
}
```

### 4.4 yield 合法性：allowYield 计数器

```cpp
// === lua_state.hpp 新增字段 ===

class LuaState {
    // ...
private:
    /// 允许 yield 的嵌套层数
    /// > 0 表示当前可以 yield
    /// == 0 表示不可以 yield（在 pcall/metamethod/C-call 中）
    u16 allowYield_ = 0;

public:
    void incAllowYield() noexcept { allowYield_++; }
    void decAllowYield() noexcept { if (allowYield_ > 0) allowYield_--; }
    bool canYield() const noexcept { return allowYield_ > 0; }

    /// yield 相关
    void setYieldResults(i32 n) noexcept { yieldResults_ = n; }
    i32  getYieldResults() const noexcept { return yieldResults_; }

private:
    i32 yieldResults_ = 0;
};
```

**使用规则**：
- `Thread::resume()` 在调用 VM 前 `incAllowYield()`，调用后 `decAllowYield()`
- `LuaState::pcall()` 在 protected 执行前 `decAllowYield()`，执行后 `incAllowYield()`
- `vmPrecall` 中的元方法调用自动处于 `allowYield_ == 0`
- `coroutine.yield` 的 C 函数实现中检查 `L->canYield()`

### 4.5 yield 时 VM 的精确行为

yield 发生的调用链：

```
executeProto()
  → CALL 指令
    → vmPrecall() 执行 C 函数 coroutine.yield
      → yield_impl(L) 设置 L->status_ = Yield, L->yieldResults_ = n
    → vmPrecall 返回 false（C 函数已执行完成）
  → 回到 CALL 指令，检测 Yield 状态
  → return ExecResult::Yielded
→ 返回到 Thread::resume()
→ resume 搬运结果值，设置协程状态为 Suspended
```

恢复时的调用链：

```
Thread::resume()
  → 将 resume 参数放到协程栈上（作为 yield 的"返回值"）
  → 设置协程状态为 Running
  → 调用 VM::executeProto(state_, ...)
    → reentry: 从 CallInfo.savedpc 恢复 pc
    → 继续执行 CALL 之后的指令
```

**无额外 resumeExecution 入口**：直接走现有 `executeProto` → `reentry` 路径。

---

## 5. Thread 核心实现

### 5.1 Thread::create

```cpp
Thread* Thread::create(LuaState* parentL, Function* func) {
    // 1. 创建子 LuaState（共享 GlobalState，独立 Stack + CallStack）
    LuaState* coState = LuaState::newThread(parentL);

    // 2. 在协程栈上设置初始调用帧
    //    栈布局: [func]
    //    CallInfo[0] 已由 newThread 创建（虚拟主函数）
    Stack& stack = coState->getStack();
    stack.push(Value(func));
    coState->setAbsoluteTop(stack.size());

    // 3. 创建 Thread 对象
    Thread* thread = new Thread(coState);
    thread->coStatus_ = CoroutineStatus::Suspended;
    thread->firstResume_ = true;

    // 4. 注册到 GC
    parentL->getGlobalState().getGC().registerObject(thread);

    return thread;
}
```

### 5.2 Thread::resume — 核心恢复逻辑

```cpp
bool Thread::resume(LuaState* callerL, i32 nargs) {
    // ──── 前置检查 ────
    if (coStatus_ == CoroutineStatus::Dead) {
        callerL->pushBoolean(false);
        pushLuaError(callerL, "cannot resume dead coroutine");
        return false;
    }
    if (coStatus_ == CoroutineStatus::Running) {
        callerL->pushBoolean(false);
        pushLuaError(callerL, "cannot resume running coroutine");
        return false;
    }

    // ──── 值传递：callerL → coState ────
    transferValues(callerL, state_, nargs);

    if (firstResume_) {
        // 首次 resume：参数作为协程函数的参数
        // 栈上已有: [func arg1 arg2 ...]
        // 创建真正的调用帧
        setupFirstCall(state_, nargs);
        firstResume_ = false;
    } else {
        // 后续 resume：参数作为 yield 的返回值
        // VM 从 CALL 指令之后的 savedpc 恢复
        // 需要将参数安排为 C 函数 (coroutine.yield) 的返回值
        adjustYieldReturns(state_, nargs);
    }

    // ──── 状态切换 ────
    CoroutineStatus prevCallerStatus = CoroutineStatus::Dead; // 默认
    Thread* callerThread = findRunningThread(callerL);
    if (callerThread) {
        prevCallerStatus = callerThread->coStatus_;
        callerThread->coStatus_ = CoroutineStatus::Normal;  // 调用者变 normal
    }
    caller_ = callerThread;
    coStatus_ = CoroutineStatus::Running;
    state_->setStatus(ThreadStatus::OK);

    // ──── 设置 yield 许可 ────
    state_->incAllowYield();

    // 记录当前运行的 Thread
    callerL->getGlobalState().setRunningThread(this);

    // ──── 调用 VM ────
    ExecResult result;
    try {
        result = VM::executeProto(state_, getCurrentProto(), savedNexeccalls_);
    } catch (const LuaError& e) {
        // 协程内部错误 → dead
        state_->decAllowYield();
        coStatus_ = CoroutineStatus::Dead;
        restoreCallerStatus(callerThread, prevCallerStatus, callerL);

        callerL->pushBoolean(false);
        callerL->pushValue(e.getErrorObject());
        return false;
    } catch (const std::exception& e) {
        // 非 LuaError 的异常（不应发生，兜底）
        state_->decAllowYield();
        coStatus_ = CoroutineStatus::Dead;
        restoreCallerStatus(callerThread, prevCallerStatus, callerL);

        callerL->pushBoolean(false);
        auto& pool = callerL->getGlobalState().getStringPool();
        callerL->pushString(pool.intern(e.what()));
        return false;
    }

    state_->decAllowYield();

    // ──── 处理结果 ────
    if (result == ExecResult::Yielded) {
        // yield：保存执行深度，搬运 yield 值到调用者栈
        savedNexeccalls_ = state_->getSavedNexeccalls();
        coStatus_ = CoroutineStatus::Suspended;
        restoreCallerStatus(callerThread, prevCallerStatus, callerL);

        callerL->pushBoolean(true);
        transferYieldResults(state_, callerL);
        return true;
    } else {
        // 正常返回：协程完成
        coStatus_ = CoroutineStatus::Dead;
        restoreCallerStatus(callerThread, prevCallerStatus, callerL);

        callerL->pushBoolean(true);
        transferReturnValues(state_, callerL);
        return true;
    }
}
```

### 5.3 coroutine.yield 的 C 函数实现

```cpp
/// coroutine.yield 的 C 函数（注册到 coroutine 表中）
static i32 yield_impl(LuaState* L) {
    // 检查 yield 合法性
    if (!L->canYield()) {
        throw LuaError("attempt to yield across a C-call boundary");
    }

    i32 nresults = L->getTop();  // 所有参数都是 yield 值

    // 设置 yield 状态
    L->setStatus(ThreadStatus::Yield);
    L->setYieldResults(nresults);

    // 返回 0（C 函数返回值数量）
    // VM 在 CALL 指令中检测到 Yield 状态后会 return ExecResult::Yielded
    // yield 的值已经在栈上，由 Thread::resume 搬运
    return 0;
}
```

### 5.4 值搬运函数

```cpp
/// 从 src 栈顶取 n 个值，按顺序压入 dst 栈
/// 
/// GC 安全性：
///   - 只搬运 Value（tagged union），不涉及指针重新解释
///   - GCObject* 指针搬运后，两个栈都引用同一对象
///   - GC 标记时 Thread::markChildren 会遍历协程栈，确保可达性
///   - 搬运完成后 src 栈高度下降，但对象本身不被释放（GC 跟踪的是 GCObject 链表）
///
static void transferValues(LuaState* src, LuaState* dst, i32 n) {
    if (n <= 0) return;

    Stack& srcStack = src->getStack();
    usize srcTop = src->getAbsoluteTop();
    usize start = srcTop - static_cast<usize>(n);

    for (usize i = start; i < srcTop; i++) {
        dst->pushValue(srcStack.at(i));
    }

    // 从 src 栈上移除
    src->setAbsoluteTop(start);
}

/// yield 后将结果值从协程栈搬到调用者栈
/// yield 值位于协程栈的 CALL 指令对应的参数区域
static void transferYieldResults(LuaState* coState, LuaState* callerL) {
    i32 nresults = coState->getYieldResults();
    // yield 值在 C 函数（yield_impl）被调用时压在参数区
    // vmPrecall 为 C 函数创建了 CallInfo，现在需要从那个帧中取值
    
    CallInfo& ci = coState->getCurrentCallInfo();
    Stack& stack = coState->getStack();
    
    for (i32 i = 0; i < nresults; i++) {
        callerL->pushValue(stack.at(ci.base + static_cast<usize>(i)));
    }
}

/// 后续 resume 时将参数安排为 yield 的"返回值"
///
/// 语义：resume(co, a, b) → coroutine.yield() 返回 a, b
///
/// 实现：参数已被 transferValues 压入协程栈顶。
///       vm 从 savedpc 恢复后，CALL 指令的 postcall 环节会把
///       C 函数的返回值从 firstResult 位置搬到 funcPos。
///       所以我们需要将 resume 参数放到正确的位置，模拟
///       yield C 函数的返回值。
static void adjustYieldReturns(LuaState* coState, i32 nargs) {
    // 弹出 yield 的 CallInfo（C 函数帧）
    // 然后将 resume 参数作为返回值放在 funcPos 位置
    CallInfo& yieldCI = coState->getCurrentCallInfo();
    usize funcPos = yieldCI.func;
    i32 wantedResults = yieldCI.nresults;

    coState->popCallInfo();

    Stack& stack = coState->getStack();
    usize srcTop = coState->getAbsoluteTop();
    usize argStart = srcTop - static_cast<usize>(nargs);

    // 将 resume 参数放到 funcPos 位置（模拟 vmPostcall）
    for (i32 i = 0; i < nargs; i++) {
        stack.at(funcPos + static_cast<usize>(i)) = stack.at(argStart + static_cast<usize>(i));
    }

    // 调整返回值数量
    if (wantedResults >= 0) {
        i32 actual = nargs;
        while (actual < wantedResults) {
            stack.at(funcPos + static_cast<usize>(actual)) = Value();
            actual++;
        }
        coState->setAbsoluteTop(funcPos + static_cast<usize>(wantedResults));
    } else {
        // MULTRET
        coState->setAbsoluteTop(funcPos + static_cast<usize>(nargs));
    }
}
```

### 5.5 首次 resume 的调用帧设置

```cpp
/// 首次 resume 时设置调用帧
/// 栈上已有: [虚拟nil] [func] [arg1 arg2 ... argN]
/// 需要创建 CallInfo 并准备执行
static void setupFirstCall(LuaState* coState, i32 nargs) {
    Stack& stack = coState->getStack();

    // 函数在位置 1（位置 0 是虚拟 nil）
    usize funcPos = 1;
    Value& funcVal = stack.at(funcPos);
    Function* func = funcVal.asFunction();
    Proto* proto = func->getProto();

    i32 numParams = proto->getNumParams();
    usize base;

    if (proto->isVararg()) {
        usize oldBase = funcPos + 1;
        i32 actualArgs = nargs;
        while (actualArgs < numParams) {
            stack.push(Value());
            actualArgs++;
        }
        base = oldBase + static_cast<usize>(actualArgs);
        stack.checkSpace(static_cast<usize>(numParams) + 1);
        for (i32 i = 0; i < numParams; i++) {
            stack.push(stack[oldBase + i]);
            stack[oldBase + i] = Value();
        }
    } else {
        base = funcPos + 1;
        i32 actualArgs = nargs;
        while (actualArgs < numParams) {
            stack.push(Value());
            actualArgs++;
        }
    }

    CallInfo& ci = coState->pushCallInfo();
    ci.func = funcPos;
    ci.base = base;
    ci.top = base + proto->getMaxStackSize();
    ci.nresults = MULTRET;
    ci.savedpc = nullptr;
    ci.tailcalls = 0;

    while (stack.size() < ci.top) stack.push(Value());
    coState->setAbsoluteTop(ci.top);
}
```

---

## 6. LuaState 新增方法

```cpp
// === lua_state.hpp 新增 ===

class LuaState {
public:
    /// 创建子线程（协程用）
    /// 共享 GlobalState，共享 globalTable（默认），独立 Stack + CallStack
    static LuaState* newThread(LuaState* parentL);

    // yield 许可
    void incAllowYield() noexcept { allowYield_++; }
    void decAllowYield() noexcept { if (allowYield_ > 0) allowYield_--; }
    bool canYield() const noexcept { return allowYield_ > 0; }

    // yield 值数量
    void setYieldResults(i32 n) noexcept { yieldResults_ = n; }
    i32  getYieldResults() const noexcept { return yieldResults_; }

    // nexeccalls 保存/恢复
    void setSavedNexeccalls(i32 n) noexcept { savedNexeccalls_ = n; }
    i32  getSavedNexeccalls() const noexcept { return savedNexeccalls_; }

    // 当前 LuaState 对应的 Thread 对象（主线程为 nullptr）
    Thread* getThread() const noexcept { return thread_; }
    void setThread(Thread* t) noexcept { thread_ = t; }

private:
    u16     allowYield_ = 0;
    i32     yieldResults_ = 0;
    i32     savedNexeccalls_ = 1;
    Thread* thread_ = nullptr;   // 所属 Thread（主线程为 nullptr）
};
```

### 6.1 LuaState::newThread 实现

```cpp
LuaState* LuaState::newThread(LuaState* parentL) {
    LuaState* L = new LuaState();

    // 共享全局表
    L->globalTable_ = parentL->globalTable_;

    // 注意：不调用 initialize()（那会创建新的全局表并注册为 GC 根）
    // 初始化调用栈
    CallInfo& ci = L->callStack_[0];
    ci.func = 0;
    ci.base = 1;
    ci.top = MIN_STACK_SIZE;
    ci.savedpc = nullptr;
    ci.nresults = MULTRET;
    ci.tailcalls = 0;

    // 虚拟主函数位
    L->stack_.push(Value());  // nil
    L->top_ = 1;

    L->status_ = ThreadStatus::OK;

    return L;
}
```

---

## 7. resume 链与 normal 状态

### 7.1 问题

当协程 A resume 协程 B 时：
- A 的状态从 `Running` → `Normal`
- B 的状态从 `Suspended` → `Running`
- B yield 或完成时，A 恢复为 `Running`

当嵌套 resume 时（A → B → C）：
- A: Normal, B: Normal, C: Running
- C yield → B: Running, A: Normal, C: Suspended
- B yield → A: Running, B: Suspended

### 7.2 实现

通过 `Thread::caller_` 形成单向链表：

```
主线程 → A(Running) → resume B
   A.caller_ = nullptr (主线程不是 Thread)
   B.caller_ = A (或通过 GlobalState 追踪)
   A.status = Normal
   B.status = Running

B → resume C
   C.caller_ = B
   B.status = Normal
   C.status = Running
```

恢复时：
```
C yield:
   C.status = Suspended
   B.status = Running  (从 B.caller_ 恢复)

B yield:
   B.status = Suspended
   A.status = Running  (从 A.caller_ 恢复)
```

### 7.3 coroutine.status 的 normal 判断

```cpp
static i32 coroutine_status(LuaState* L) {
    Thread* thread = getThreadArg(L);

    const char* status;
    switch (thread->getCoroutineStatus()) {
        case CoroutineStatus::Suspended: status = "suspended"; break;
        case CoroutineStatus::Running:   status = "running";   break;
        case CoroutineStatus::Normal:    status = "normal";    break;
        case CoroutineStatus::Dead:      status = "dead";      break;
    }

    pushString(L, status);
    return 1;
}
```

### 7.4 GlobalState 新增：运行中 Thread 追踪

```cpp
// === global_state.hpp 新增 ===

class GlobalState {
public:
    Thread* getRunningThread() const noexcept { return runningThread_; }
    void setRunningThread(Thread* t) noexcept { runningThread_ = t; }

private:
    Thread* runningThread_ = nullptr;  // 当前正在执行的协程（主线程时为 nullptr）
};
```

---

## 8. GC 集成

### 8.1 Thread::markChildren

```cpp
void Thread::markChildren(GarbageCollector& gc) {
    // 标记协程 LuaState 栈上的所有值
    Stack& stack = state_->getStack();
    usize top = state_->getAbsoluteTop();
    for (usize i = 0; i < top; i++) {
        Value& v = stack.at(i);
        if (v.isCollectable()) {
            gc.markValue(v);
        }
    }

    // 标记所有 CallInfo 中引用的函数
    for (usize i = 0; i <= state_->getCurrentCI(); i++) {
        CallInfo& ci = state_->getCallStack()[i];
        if (ci.func < top) {
            Value& funcVal = stack.at(ci.func);
            if (funcVal.isCollectable()) {
                gc.markValue(funcVal);
            }
        }
    }

    // 标记 open upvalues
    Upvalue* uv = state_->getOpenUpvalues();
    while (uv) {
        gc.markObject(uv);
        uv = uv->getNext();
    }

    // 注意：不标记 globalTable_（共享表由主 LuaState 标记）
    // 注意：不标记 caller_（caller 有自己的根引用或 Thread 引用）
}

void Thread::destroy() {
    delete state_;
    state_ = nullptr;
}

usize Thread::getMemorySize() const {
    // Thread 对象 + LuaState 估算
    return sizeof(Thread) + sizeof(LuaState)
         + state_->getStack().capacity() * sizeof(Value)
         + state_->getCallStack().capacity() * sizeof(CallInfo);
}
```

### 8.2 GC 关键规则

1. **Thread 是 GCObject**：和 Table/Function 一样参与标记-清除
2. **协程栈上的值必须被标记**：否则只在协程栈上引用的对象会被错误回收
3. **suspended 协程不会阻止 GC**：如果 Thread 本身不可达，GC 会回收它（包括释放协程栈）
4. **值搬运无需写屏障**：当前 GC 是 stop-the-world 标记清除，不需要 incremental barrier

---

## 9. 标准库函数实现

### 9.1 文件：src/lib/coroutinelib.cpp

```cpp
/// coroutine.create(f) → thread
static i32 coroutine_create(LuaState* L) {
    if (L->getTop() < 1 || !L->at(-1).isFunction()) {
        throw LuaError("bad argument #1 to 'create' (function expected)");
    }

    Function* func = L->at(-1).asFunction();
    Thread* thread = Thread::create(L, func);

    L->pop(1);
    L->pushValue(Value(thread));
    return 1;
}

/// coroutine.resume(co, ...) → true, ... | false, err
static i32 coroutine_resume(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1 || !L->at(1).isThread()) {
        throw LuaError("bad argument #1 to 'resume' (coroutine expected)");
    }

    // 获取 Thread 对象（位于参数 1）
    Thread* thread = L->at(1).asThread();
    i32 resumeArgs = nargs - 1;  // 除去 co 参数

    // 移除 co 参数，只留 resume 参数在栈顶
    // 实际实现中需要调整栈布局
    // ...

    thread->resume(L, resumeArgs);

    // resume 内部已经 push 了 true/false + 结果值
    return L->getTop();  // 返回所有值
}

/// coroutine.yield(...) → ...
static i32 coroutine_yield(LuaState* L) {
    if (!L->canYield()) {
        throw LuaError("attempt to yield from outside a coroutine");
    }

    i32 nresults = L->getTop();
    L->setStatus(ThreadStatus::Yield);
    L->setYieldResults(nresults);
    return 0;
}

/// coroutine.status(co) → string
static i32 coroutine_status(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isThread()) {
        throw LuaError("bad argument #1 to 'status' (coroutine expected)");
    }

    Thread* thread = L->at(1).asThread();
    const char* s;
    switch (thread->getCoroutineStatus()) {
        case CoroutineStatus::Suspended: s = "suspended"; break;
        case CoroutineStatus::Running:   s = "running";   break;
        case CoroutineStatus::Normal:    s = "normal";    break;
        case CoroutineStatus::Dead:      s = "dead";      break;
        default: s = "unknown"; break;
    }

    L->pop(1);
    auto& pool = L->getGlobalState().getStringPool();
    L->pushString(pool.intern(s));
    return 1;
}

/// coroutine.wrap(f) → iterator function
static i32 coroutine_wrap(LuaState* L) {
    // 与 create 类似，但返回一个 C 闭包
    // 该闭包持有 Thread 作为 upvalue
    // 每次调用等价于 resume，但错误时直接 throw 而非返回 false

    if (L->getTop() < 1 || !L->at(-1).isFunction()) {
        throw LuaError("bad argument #1 to 'wrap' (function expected)");
    }

    Function* func = L->at(-1).asFunction();
    Thread* thread = Thread::create(L, func);

    // 创建包装闭包
    // 需要 Function 支持 C closure + upvalue（待确认现有支持度）
    // 暂定方案：利用现有 Function 类的 upvalue 机制
    // 如果不支持，Phase 1 先不实现 wrap

    // TODO: 实现 C closure with upvalue
    throw LuaError("coroutine.wrap: not yet implemented");
    return 0;
}

/// coroutine.running() → thread | nil
static i32 coroutine_running(LuaState* L) {
    Thread* running = L->getGlobalState().getRunningThread();
    if (running) {
        L->pushValue(Value(running));
    } else {
        L->pushNil();
    }
    return 1;
}

/// 注册协程库
void openCoroutineLib(LuaState* L) {
    auto& pool = L->getGlobalState().getStringPool();
    Table* coroutineTable = new Table();

    struct Entry { const char* name; CFunction func; };
    Entry funcs[] = {
        {"create",  coroutine_create},
        {"resume",  coroutine_resume},
        {"yield",   coroutine_yield},
        {"status",  coroutine_status},
        {"wrap",    coroutine_wrap},
        {"running", coroutine_running},
    };

    for (auto& e : funcs) {
        GCString* name = pool.intern(e.name);
        Function* f = new Function(e.func);
        coroutineTable->rawSet(Value(name), Value(f));
    }

    GCString* libName = pool.intern("coroutine");
    L->getGlobalTable()->rawSet(Value(libName), Value(coroutineTable));
}
```

---

## 10. LuaError 统一错误模型

### 10.1 迁移策略

当前 VM 内所有错误点使用 `throw std::runtime_error(...)`，总计约 50+ 处。

**阶段性迁移**：
- Phase 1（协程）：在 `pcall`、`Thread::resume`、`coroutine.yield` 中同时捕获 `LuaError` 和 `std::exception`
- Phase 2（后续清理）：逐步将 VM 中的 `std::runtime_error` 替换为 `LuaError`

这确保协程实现不会被错误模型迁移阻塞，同时为未来统一错误模型铺路。

### 10.2 pcall 修改

```diff
 // LuaState::pcall 的 catch 块
-    } catch (const std::exception& e) {
+    } catch (const LuaError& e) {
+        restoreCallFrames();
+        restoreStackPrefix(savedStack);
+        pushValue(e.getErrorObject());  // 直接使用 Lua Value 作为错误
+        setStatus(ThreadStatus::OK);
+        return LUA_ERRRUN;
+    } catch (const std::exception& e) {
         restoreCallFrames();
         restoreStackPrefix(savedStack);
         auto& pool = getGlobalState().getStringPool();
         pushString(pool.intern(e.what()));
         setStatus(ThreadStatus::OK);
         return LUA_ERRRUN;
     }
```

---

## 11. C++20 协程：第二阶段宿主调度层

> **Phase 1 不引入 C++20 协程。** 以下是 Phase 3 的前瞻设计。

### 11.1 C++20 协程的正确角色

C++20 协程**不负责保存 Lua 执行现场**（那是 LuaState/CallInfo 的职责），而是提供：

1. **宿主侧异步调度**：让 C++ 宿主代码可以 `co_await` Lua 协程完成
2. **async IO 桥接**：将网络/文件/定时器的异步操作桥接为 Lua yield/resume
3. **调度器集成**：与 event loop / 线程池对接

### 11.2 预留接口

```cpp
// 未来 Phase 3 的概念代码

/// C++20 awaitable，用于宿主代码等待 Lua 协程
struct LuaCoroutineAwaitable {
    Thread* thread;
    LuaState* callerL;
    i32 nargs;

    bool await_ready() { return thread->isDead(); }

    void await_suspend(std::coroutine_handle<> h) {
        // 在调度器中注册：当协程 yield 需要外部资源时，
        // 由调度器负责恢复
        scheduler->enqueue(thread, h);
    }

    ResumeResult await_resume() {
        return thread->getLastResult();
    }
};

/// 宿主使用方式
Task<void> hostFunction(LuaState* L) {
    Thread* co = Thread::create(L, func);
    auto result = co_await LuaCoroutineAwaitable{co, L, 0};
    // ...
}
```

### 11.3 扩展 yield 类型

为 async IO 预留 yield 原因标记：

```cpp
/// yield 原因（Phase 3 扩展）
enum class YieldReason : u8 {
    Explicit,     // coroutine.yield() 显式挂起
    AsyncIO,      // await IO 操作
    Timer,        // await sleep/timer
    RPC           // await RPC 调用
};
```

---

## 12. 实施阶段规划

### Phase 1：VM 状态机 + 基础协程（目标：能跑简单 yield/resume）

| 步骤 | 内容 | 预估行数 |
|------|------|---------|
| 1.1 | `LuaError` 类 | ~50 |
| 1.2 | `ExecResult` 枚举 + `executeProto` 返回类型改 `void` → `ExecResult` | ~30 |
| 1.3 | `LuaState::newThread()` 工厂方法 | ~40 |
| 1.4 | `LuaState` 新增字段（`allowYield_`, `yieldResults_`, `thread_`） | ~30 |
| 1.5 | `Thread` 类完整实现（含 GC） | ~250 |
| 1.6 | VM CALL 路径增加 yield 检测 | ~15 |
| 1.7 | `coroutine.create/resume/yield/status/running` 5 函数 | ~200 |
| 1.8 | `lib_manager` 注册协程库 | ~10 |
| 1.9 | 基础单元测试 | ~200 |
| **小计** | | **~825** |

### Phase 2：健壮性 + 完备语义

| 步骤 | 内容 |
|------|------|
| 2.1 | 嵌套协程 resume 链 + `normal` 状态 |
| 2.2 | `coroutine.wrap` 实现（需要 C closure + upvalue） |
| 2.3 | 错误传播完善（协程内 error → resume 返回 false） |
| 2.4 | pcall 内 yield 拦截（pcall 中不允许 yield） |
| 2.5 | 边界测试：dead resume、主线程 yield、递归 resume 自身 |
| 2.6 | TFORLOOP 中的 Lua 迭代器 + yield 支持 |

### Phase 3：C++20 协程调度层（未来）

| 步骤 | 内容 |
|------|------|
| 3.1 | `CoroutinePromise` + 宿主 awaitable |
| 3.2 | async IO 桥接（yield → event loop → resume） |
| 3.3 | 调度器集成 |

---

## 13. 兼容性与影响面矩阵

| 模块 | 影响 | 详情 |
|------|------|------|
| **vm.hpp** | 🟡 改动 | `ExecResult` 枚举；`executeProto` 返回类型 |
| **vm.cpp** | 🟡 改动 | CALL 增加 yield 检测(~15行)；RETURN 改为 `return ExecResult` |
| **lua_state.hpp** | 🟡 改动 | 新增 `newThread()`、yield 字段、`allowYield_` |
| **lua_state.cpp** | 🟡 改动 | `newThread()` 实现；`pcall` 增加 `LuaError` catch |
| **Value** | 🟢 无改动 | `Thread*` 变体已存在 |
| **GCObject** | 🟢 无改动 | Thread 继承即可 |
| **GarbageCollector** | 🟡 小改动 | markChildren 处理 Thread |
| **lib_manager** | 🟢 微改动 | 注册 coroutine |
| **Stack / CallInfo** | 🟢 无改动 | |
| **编译器** | 🟢 无改动 | 无需新字节码 |
| **其他标准库** | 🟢 无改动 | |
| **现有测试** | 🟢 无影响 | `ExecResult::Returned` 等效于原 `void` |

---

## 14. 验收用例

### 14.1 基础 resume/yield

```lua
local co = coroutine.create(function(a, b)
    coroutine.yield(a + b)
    return "done"
end)

local ok, val = coroutine.resume(co, 1, 2)
assert(ok == true)
assert(val == 3)
assert(coroutine.status(co) == "suspended")

ok, val = coroutine.resume(co)
assert(ok == true)
assert(val == "done")
assert(coroutine.status(co) == "dead")
```

### 14.2 多次 yield（生成器模式）

```lua
local gen = coroutine.create(function()
    for i = 1, 5 do
        coroutine.yield(i)
    end
end)

for i = 1, 5 do
    local ok, v = coroutine.resume(gen)
    assert(ok and v == i)
end

local ok = coroutine.resume(gen) -- 函数返回，无更多值
assert(ok == true)
assert(coroutine.status(gen) == "dead")
```

### 14.3 resume 参数 → yield 返回值

```lua
local co = coroutine.create(function()
    local a, b = coroutine.yield()
    return a + b
end)

coroutine.resume(co)           -- 首次启动
local ok, val = coroutine.resume(co, 10, 20)  -- 10,20 作为 yield 的返回值
assert(ok == true)
assert(val == 30)
```

### 14.4 协程内错误

```lua
local co = coroutine.create(function()
    error("boom")
end)

local ok, err = coroutine.resume(co)
assert(ok == false)
assert(type(err) == "string")
assert(coroutine.status(co) == "dead")
```

### 14.5 dead 协程不可 resume

```lua
local co = coroutine.create(function() end)
coroutine.resume(co)  -- 函数立即返回
assert(coroutine.status(co) == "dead")

local ok, err = coroutine.resume(co)
assert(ok == false)
-- err 包含 "cannot resume dead coroutine"
```

### 14.6 嵌套协程 + normal 状态

```lua
local inner, outer

inner = coroutine.create(function()
    assert(coroutine.status(outer) == "normal")
    coroutine.yield()
end)

outer = coroutine.create(function()
    coroutine.resume(inner)
end)

coroutine.resume(outer)
```

### 14.7 coroutine.running

```lua
assert(coroutine.running() == nil)  -- 主线程

local co = coroutine.create(function()
    assert(coroutine.running() ~= nil)
end)
coroutine.resume(co)
```

---

## 15. 总结

本 v2 方案的核心改进：

1. **VM 显式返回 `ExecResult::Yielded`** 而非抛异常 —— yield 是正常控制流，不是错误
2. **所有执行现场由 LuaState/CallInfo 持有** —— 不依赖 C++ 协程帧
3. **复用 `reentry` 标签恢复执行** —— 无需新的 `resumeExecution` 入口
4. **`allowYield` 计数器** 精确控制 yield 合法性
5. **resume 链追踪** 支持嵌套协程和 `normal` 状态
6. **`LuaError` 统一错误类型** 兼容现有 `std::exception` 并为未来铺路
7. **C++20 协程推迟到 Phase 3** 作为宿主调度层，不用于保存 Lua 执行现场

> **等待评审**：请确认此方案后，我将按 Phase 1 步骤开始实现。
