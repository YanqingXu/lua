# Lua 协程实现技术方案：基于 C++20 Coroutines

> **目标**：在现有解释器架构上，使用 C++20 协程（`<coroutine>`）实现 Lua 5.1.5 完整的协程语义。

---

## 1. Lua 5.1.5 协程语义回顾

### 1.1 API 规范

| 函数 | 签名 | 语义 |
|------|------|------|
| `coroutine.create(f)` | `function → thread` | 以函数 `f` 创建新协程，返回 thread 对象，状态为 `suspended` |
| `coroutine.resume(co, ...)` | `thread, ... → ok, ...` | 恢复协程执行，首次调用传入参数给 `f`，后续调用的参数作为 `yield` 的返回值 |
| `coroutine.yield(...)` | `... → ...` | 挂起当前协程，参数作为 `resume` 的返回值；下次 `resume` 的参数作为本次 `yield` 的返回值 |
| `coroutine.status(co)` | `thread → string` | 返回 `"suspended"` / `"running"` / `"dead"` / `"normal"` |
| `coroutine.wrap(f)` | `function → function` | 创建协程并返回一个迭代器函数，每次调用等价于 `resume`（出错时直接抛异常而非返回 false） |
| `coroutine.running()` | `→ thread or nil` | 返回当前正在执行的协程（主线程时返回 nil） |

### 1.2 关键语义约束

1. **对称性**：`resume` 的参数 → `yield` 的返回值；`yield` 的参数 → `resume` 的返回值
2. **错误传播**：协程内部的未捕获错误通过 `resume` 返回 `false, errmsg`
3. **生命周期**：函数正常返回后协程变为 `dead`，不可再 resume
4. **嵌套约束**：不支持跨协程 yield（C 函数边界不可 yield，Lua 5.1 限制）
5. **主线程**：主线程不是可 yield 的协程

### 1.3 状态机

```
                create(f)
    ┌──────────────────────────┐
    │                          ▼
    │    ┌──────────┐    ┌───────────┐
    │    │  normal   │    │ suspended │ ◄─── yield()
    │    └────┬─────┘    └─────┬─────┘
    │         │                │
    │    被其他co              resume()
    │    resume时              │
    │    自动设置              │
    │         │                ▼
    │    ┌────┴─────┐    ┌───────────┐
    │    │  normal   │◄───│  running  │
    │    └──────────┘    └─────┬─────┘
    │                          │
    │                    return / error
    │                          │
    │                          ▼
    │                    ┌───────────┐
    └────────────────────│   dead    │
                         └───────────┘
```

---

## 2. 为什么选择 C++20 协程

### 2.1 项目现状

- 编译配置已为 `stdcpp20`（`lua.vcxproj`, `lua_app.vcxproj` 等），部分配置甚至为 `stdcpp23`
- 编译器为 MSVC (Visual Studio 2026)，对 C++20 协程支持完善
- 当前 `Thread` 类仅有前向声明（`value.hpp:34`），尚无实现
- `ThreadStatus::Yield` 枚举值已预留（`lua_state.hpp:48`）
- `Value` 已包含 `Thread*` 变体（索引 8），`isThread()` / `asThread()` 方法已实现

### 2.2 C++20 协程 vs 传统方案对比

| 方案 | 实现方式 | 优势 | 劣势 |
|------|---------|------|------|
| **C++20 协程** | `co_await` / `co_yield` / `co_return` | 编译器原生支持；栈帧自动管理；代码直觉性强 | 需要理解 promise_type 机制；协程帧生命周期需手动管理 |
| **setjmp/longjmp** | C 风格非局部跳转 | 传统、与原版 Lua 一致 | 不安全，无法正确析构 C++ 对象；MSVC SEH 冲突 |
| **Fiber/UMS** | Windows Fiber API | 真正的栈切换 | 平台绑定、不可移植、API 陈旧 |
| **手动状态机** | 将 VM 执行循环改为可中断状态机 | 纯 C++，无特殊依赖 | 大规模重构 VM 执行引擎；代码复杂性高 |

**选择 C++20 协程的核心理由**：
1. 项目已启用 C++20，无额外编译开销
2. 协程帧由编译器管理，避免手动栈分配
3. 挂起/恢复语义与 Lua 的 yield/resume **天然对齐**
4. MSVC 对 C++20 协程支持成熟稳定
5. 与项目"现代 C++"设计理念一致

---

## 3. 核心架构设计

### 3.1 整体分层

```
┌─────────────────────────────────────────────────────────┐
│  Lua 脚本层:  coroutine.create / resume / yield / ...   │
├─────────────────────────────────────────────────────────┤
│  标准库层:    src/lib/coroutinelib.cpp (6个C函数注册)    │
├─────────────────────────────────────────────────────────┤
│  协程执行层:  CoroutineContext (C++20 协程 Promise)      │
├─────────────────────────────────────────────────────────┤
│  线程状态层:  Thread : GCObject (协程 = 独立 LuaState)   │
├─────────────────────────────────────────────────────────┤
│  VM 层:       executeProto() 改造为可挂起/恢复           │
├─────────────────────────────────────────────────────────┤
│  状态层:      LuaState (stack_, callStack_, top_, ...)  │
└─────────────────────────────────────────────────────────┘
```

### 3.2 关键类型定义

```cpp
// ═══════════════════════════════════════════
// 文件: src/core/thread.hpp
// ═══════════════════════════════════════════

#pragma once
#include <coroutine>
#include "core/gc_object.hpp"
#include "vm/lua_state.hpp"

namespace Lua {

/// 协程状态（Lua 层面）
enum class CoroutineStatus : u8 {
    Suspended,  // 创建后 / yield 后
    Running,    // 正在执行
    Normal,     // resume 了其他协程，自己暂停
    Dead        // 函数返回或出错
};

// 前向声明
class Thread;

/// ═══════════════════════════════════════════
/// C++20 协程的 Promise Type
/// ═══════════════════════════════════════════
struct CoroutinePromise {
    Thread* thread = nullptr;  // 回指所属 Thread 对象

    // --- 协程返回对象 ---
    struct CoroutineHandle {
        using promise_type = CoroutinePromise;
        std::coroutine_handle<CoroutinePromise> handle;

        CoroutineHandle(std::coroutine_handle<CoroutinePromise> h) : handle(h) {}
        ~CoroutineHandle() { /* Thread 析构时统一 destroy */ }

        CoroutineHandle(CoroutineHandle&& o) noexcept : handle(o.handle) {
            o.handle = nullptr;
        }
        CoroutineHandle& operator=(CoroutineHandle&& o) noexcept {
            handle = o.handle;
            o.handle = nullptr;
            return *this;
        }

        // 禁止拷贝
        CoroutineHandle(const CoroutineHandle&) = delete;
        CoroutineHandle& operator=(const CoroutineHandle&) = delete;
    };

    // --- promise_type 必须方法 ---
    CoroutineHandle get_return_object() {
        return { std::coroutine_handle<CoroutinePromise>::from_promise(*this) };
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }

    void return_void() {}
    void unhandled_exception() {
        // 捕获异常，存入 Thread 的错误状态
        thread->storeException(std::current_exception());
    }
};

/// ═══════════════════════════════════════════
/// Thread 类：GC 管理的协程对象
/// ═══════════════════════════════════════════
///
/// 每个 Thread 拥有：
///   - 独立的 LuaState（独立栈 + 调用栈）
///   - 一个 C++20 协程句柄（管理挂起/恢复）
///   - Lua 层面的协程状态
///
class Thread : public GCObject {
public:
    /// 创建新协程线程，func 是协程要执行的 Lua 函数
    static Thread* create(LuaState* parentL, Function* func);

    /// 销毁协程（释放协程帧 + LuaState）
    ~Thread();

    // --- 核心协程操作 ---

    /// resume：恢复协程执行
    ///   - 首次: 传入参数给协程函数
    ///   - 后续: 传入参数作为上次 yield 的返回值
    ///   返回: true=正常挂起/完成, false=错误
    bool resume(LuaState* callerL, i32 nargs);

    /// yield：从协程内部挂起（由 VM 层调用）
    ///   - 将栈上 nresults 个值作为 resume 的返回值
    static void yield(LuaState* L, i32 nresults);

    // --- 状态查询 ---
    CoroutineStatus getCoroutineStatus() const noexcept { return coStatus_; }
    LuaState* getLuaState() const noexcept { return state_; }
    bool isDead() const noexcept { return coStatus_ == CoroutineStatus::Dead; }
    bool isSuspended() const noexcept { return coStatus_ == CoroutineStatus::Suspended; }

    // --- GCObject 接口 ---
    void markChildren(GarbageCollector& gc) override;
    usize getMemorySize() const override;
    void destroy() override;

    // --- 异常存储 ---
    void storeException(std::exception_ptr eptr) { exception_ = eptr; }
    std::exception_ptr getException() const { return exception_; }

private:
    Thread(LuaState* state);

    LuaState*       state_;       // 协程独立的 LuaState
    CoroutineStatus coStatus_;    // Lua 层协程状态

    // C++20 协程句柄
    CoroutinePromise::CoroutineHandle coHandle_;

    // 调用者引用（resume 时设置，用于 yield 回传）
    LuaState* callerState_ = nullptr;

    // 错误存储
    std::exception_ptr exception_;
};

} // namespace Lua
```

---

## 4. C++20 协程集成方案

### 4.1 核心思路：VM 执行循环作为协程体

传统实现中，`VM::executeProto()` 是一个**不可中断的循环**。我们需要将其改造为**可在特定点挂起的 C++20 协程**。

关键洞察：**不需要将整个 VM 循环变成协程**。Lua 5.1 的协程 yield 只能发生在 Lua 函数调用（非 C 调用）边界，因此我们可以采用一种更轻量的方案。

### 4.2 方案选择：Wrapper 协程模式

```
                    ┌──────────────────────────────────────┐
                    │  C++20 协程帧 (coroutineBody)         │
                    │                                      │
  resume ──────────►│  初始化协程 LuaState                  │
                    │  调用 VM::executeProto(coState, ...)  │
                    │         │                             │
                    │         │ ◄── VM 内部遇到 yield 指令   │
                    │         │     设置状态，抛出 YieldSignal│
                    │         │                             │
                    │  catch (YieldSignal&) ──►  co_await   │ ──► 挂起
                    │                           suspend     │
                    │         │                             │
  resume ──────────►│  恢复执行                             │
                    │  重新调用 VM::executeProto(coState)    │
                    │    (从保存的 PC/CallInfo 恢复)         │
                    │         │                             │
                    │  函数正常返回 ──► co_return            │ ──► 完成
                    └──────────────────────────────────────┘
```

#### 4.2.1 YieldSignal 方式（推荐方案 A）

```cpp
/// yield 信号，用于从 VM 执行循环中跳出
struct YieldSignal {
    i32 nresults;  // yield 的返回值数量
};

/// 协程体函数（C++20 协程）
CoroutinePromise::CoroutineHandle coroutineBody(Thread* thread) {
    LuaState* L = thread->getLuaState();

    // 首次 resume 到达这里
    co_await std::suspend_always{};  // initial_suspend 已处理，这里是等待首次 resume

    while (true) {
        try {
            // 从协程 LuaState 的当前调用帧恢复执行
            Function* func = /* 从 L 的栈帧获取 */;
            VM::executeProto(L, func->getProto(), 1);

            // 正常返回 → 协程完成
            break;

        } catch (YieldSignal& ys) {
            // yield 触发：
            // 1. VM 已将 yield 值放到协程栈上
            // 2. VM 已保存 PC 和 CallInfo 状态
            // 3. 挂起 C++20 协程帧
            co_await std::suspend_always{};
            // resume 后：
            // 4. 调用者已将 resume 参数放到协程栈上
            // 5. 继续循环，重新进入 VM 继续执行
        }
    }

    co_return;
}
```

#### 4.2.2 VM 执行循环中的 yield 处理

```cpp
// 在 vm.cpp 的 CALL 指令处理中，增加对 yield 的识别

// 当 C 函数（coroutine.yield 的实现）设置了 Yield 状态时：
case OpCode::CALL: {
    // ... 调用函数 ...

    // 检查是否 yield
    if (L->getStatus() == ThreadStatus::Yield) {
        // 保存当前执行状态到 CallInfo
        ci.savedpc = &code[pc];
        // 抛出 yield 信号，让 C++20 协程帧捕获
        throw YieldSignal{ L->getYieldResults() };
    }
    // ... 正常继续 ...
}
```

### 4.3 替代方案 B：纯 Awaitable 模式（更激进）

> 说明：此方案改动更大，但更"正统"地使用 C++20 协程，作为备选。

将 `executeProto` 本身变为协程，遇到 yield 时直接 `co_await`：

```cpp
// 概念代码，需要大幅改造 VM

CoTask VM::executeProtoAsync(LuaState* L, Proto* proto, i32 nexeccalls) {
    // ... 初始化 ...

    while (pc < code.size()) {
        switch (GET_OPCODE(inst)) {
            case OpCode::CALL: {
                if (isYielding) {
                    co_await YieldAwaitable{};  // 直接挂起
                }
                break;
            }
            // ... 其他指令 ...
        }
    }
}
```

**不推荐此方案的原因**：
- 需要将 VM 执行引擎的返回类型从 `void` 改为协程返回类型
- 嵌套的 `executeProto` 调用也需要变成协程，影响面过大
- 函数调用链（vmPrecall → executeProto → ...）全部需要协程化

### 4.4 推荐方案 A 的优势总结

| 维度 | 方案 A (YieldSignal) | 方案 B (纯 Awaitable) |
|------|---------------------|----------------------|
| VM 改动量 | 小——仅添加 yield 检测和 throw | 大——所有执行函数改为协程 |
| 与现有代码兼容 | 高——executeProto 签名不变 | 低——返回类型全部变化 |
| 性能 | yield 路径用 exception，正常路径无开销 | 每次函数调用有协程开销 |
| 正确性 | yield 时通过 exception 展开调用栈，恢复时重新进入 | 直接挂起/恢复，语义更精确 |
| 实现难度 | 中等 | 高 |

---

## 5. 详细实现计划

### 5.1 新增文件清单

| 文件 | 职责 |
|------|------|
| `src/core/thread.hpp` | Thread 类声明 + CoroutinePromise 定义 |
| `src/core/thread.cpp` | Thread 类实现（create, resume, yield, GC） |
| `src/lib/coroutinelib.hpp` | 协程库头文件 |
| `src/lib/coroutinelib.cpp` | 6 个标准库函数实现 |
| `tests/unit/stdlib/test_coroutinelib.cpp` | 协程单元测试 |

### 5.2 需修改的现有文件

| 文件 | 修改内容 |
|------|---------|
| `src/vm/vm.cpp` | 在 `CALL`/`RETURN` 路径增加 yield 检测；增加 `YieldSignal` 异常 |
| `src/vm/vm.hpp` | 增加 `YieldSignal` 定义 |
| `src/vm/lua_state.hpp/cpp` | 增加 `yieldResults_` 字段、`createThread()` 工厂方法 |
| `src/core/value.hpp` | 去掉 `Thread` 前向声明的空壳，改为 include `thread.hpp`（或保持前向声明 + 在 .cpp 中 include） |
| `src/gc/garbage_collector.cpp` | 在标记阶段处理 Thread 对象（标记其 LuaState 栈上所有值） |
| `src/lib/lib_manager.cpp` | 注册 coroutine 库 |
| `src/common/config.hpp` | 添加协程相关配置常量（如 `LUA_DEFAULT_COROUTINE_STACK_SIZE`） |

### 5.3 Thread 类核心实现

```cpp
// ═══════════════════════════════════════════
// 文件: src/core/thread.cpp
// ═══════════════════════════════════════════

Thread* Thread::create(LuaState* parentL, Function* func) {
    // 1. 创建子 LuaState（共享 GlobalState，独立 Stack + CallStack）
    LuaState* coState = LuaState::newThread(parentL);

    // 2. 将函数压入协程栈
    coState->pushValue(Value(func));

    // 3. 创建 Thread 对象
    Thread* thread = new Thread(coState);

    // 4. 创建 C++20 协程帧
    thread->coHandle_ = coroutineBody(thread);

    // 5. 注册到 GC
    parentL->getGlobalState().getGC().addObject(thread);

    // 6. 初始状态: suspended
    thread->coStatus_ = CoroutineStatus::Suspended;

    return thread;
}

bool Thread::resume(LuaState* callerL, i32 nargs) {
    // 前置检查
    if (coStatus_ == CoroutineStatus::Dead) {
        callerL->pushString("cannot resume dead coroutine");
        return false;
    }
    if (coStatus_ == CoroutineStatus::Running) {
        callerL->pushString("cannot resume running coroutine");
        return false;
    }

    // 1. 将 resume 参数从 callerL 栈转移到 coState 栈
    transferValues(callerL, state_, nargs);

    // 2. 设置状态
    coStatus_ = CoroutineStatus::Running;
    callerState_ = callerL;
    state_->setStatus(ThreadStatus::OK);

    // 3. 恢复 C++20 协程
    coHandle_.handle.resume();

    // 4. 检查结果
    if (exception_) {
        // 协程内部出错 → dead
        coStatus_ = CoroutineStatus::Dead;
        callerL->pushBoolean(false);
        // 将错误消息传回调用者
        try {
            std::rethrow_exception(exception_);
        } catch (const std::exception& e) {
            callerL->pushString(e.what());
        }
        exception_ = nullptr;
        return false;
    }

    if (coHandle_.handle.done()) {
        // 协程函数正常返回 → dead
        coStatus_ = CoroutineStatus::Dead;
    } else {
        // yield → suspended
        coStatus_ = CoroutineStatus::Suspended;
    }

    // 5. 将返回值从 coState 栈转移到 callerL 栈
    callerL->pushBoolean(true);  // success flag
    transferResults(state_, callerL);

    callerState_ = nullptr;
    return true;
}

void Thread::yield(LuaState* L, i32 nresults) {
    // 此函数由 coroutine.yield 的 C 函数实现调用
    // 作用：设置标志，让 VM 知道需要 yield

    L->setStatus(ThreadStatus::Yield);
    L->setYieldResults(nresults);

    // 实际的挂起由 VM 层检测后抛出 YieldSignal 完成
    // YieldSignal 被 coroutineBody 中的 catch 捕获
    // 然后 co_await 挂起 C++20 协程帧
}
```

### 5.4 LuaState 新增方法

```cpp
// --- lua_state.hpp 新增 ---

class LuaState {
public:
    /// 创建子线程（协程用），共享 GlobalState，独立栈
    static LuaState* newThread(LuaState* parentL);

    /// yield 相关
    void setYieldResults(i32 n) noexcept { yieldResults_ = n; }
    i32  getYieldResults() const noexcept { return yieldResults_; }

    /// 当前运行的协程
    Thread* getRunningThread() const noexcept { return runningThread_; }
    void setRunningThread(Thread* t) noexcept { runningThread_ = t; }

private:
    i32     yieldResults_ = 0;       // yield 返回值数量
    Thread* runningThread_ = nullptr; // 当前运行中的协程（主线程上追踪）
};
```

### 5.5 值传递机制

resume/yield 时需要在两个 LuaState 之间传递值：

```cpp
/// 将值从 src 栈顶 n 个位置移动到 dst 栈
void transferValues(LuaState* src, LuaState* dst, i32 n) {
    // 1. 从 src 栈上取出 n 个值（从栈底到栈顶顺序）
    usize srcTop = src->getAbsoluteTop();
    usize start = srcTop - static_cast<usize>(n);

    for (usize i = start; i < srcTop; i++) {
        dst->pushValue(src->getStack().at(i));
    }

    // 2. 从 src 栈上移除这些值
    src->setAbsoluteTop(start);
}
```

### 5.6 VM 层 yield 检测（vm.cpp 修改）

```cpp
// 在 vm.cpp 中增加 YieldSignal 和检测逻辑

struct YieldSignal {
    i32 nresults;
};

// === CALL 指令处理修改 ===

case OpCode::CALL: {
    usize funcPos = ci.base + a;
    i32 nArgs = (b != 0) ? (b - 1) : /* 计算实际参数数 */;
    i32 nResults = c - 1;

    L->getCurrentCallInfo().savedpc = &code[pc];

    bool isLua = vmPrecall(L, funcPos, nArgs, nResults);

    if (isLua) {
        nexeccalls++;
        goto reentry;
    }

    // C 函数已执行完毕，检查是否触发了 yield
    if (L->getStatus() == ThreadStatus::Yield) {
        // 保存当前帧状态
        ci.savedpc = &code[pc];
        throw YieldSignal{ L->getYieldResults() };
    }

    base = refreshBase(L);
    break;
}
```

### 5.7 协程恢复时 VM 重入

yield 后恢复执行的关键：**VM 需要从保存的 CallInfo 和 PC 恢复**。

```cpp
// coroutineBody 中 catch 后重新进入 VM 的逻辑：

CoroutinePromise::CoroutineHandle coroutineBody(Thread* thread) {
    LuaState* L = thread->getLuaState();

    while (true) {
        try {
            // 恢复执行：VM 从 L 当前的 CallInfo.savedpc 继续
            // executeProto 会从 L->getCurrentCallInfo() 取出 savedpc 并恢复
            VM::resumeExecution(L);
            break;  // 正常返回 = 协程完成

        } catch (YieldSignal& ys) {
            // 保存 yield 结果数量
            L->setYieldResults(ys.nresults);
            // 挂起 C++20 协程帧，控制权回到 resume 调用处
            co_await std::suspend_always{};
            // 恢复后继续循环
        }
    }

    co_return;
}
```

这需要增加一个新的 VM 入口点：

```cpp
// vm.hpp 新增
namespace VM {
    /// 从挂起状态恢复执行（协程用）
    /// 从 L 的当前 CallInfo 中恢复 pc 和栈帧
    void resumeExecution(LuaState* L);
}
```

---

## 6. GC 集成

### 6.1 Thread 对象标记

```cpp
void Thread::markChildren(GarbageCollector& gc) {
    // 标记协程 LuaState 栈上的所有值
    Stack& stack = state_->getStack();
    for (usize i = 0; i < state_->getAbsoluteTop(); i++) {
        Value& v = stack.at(i);
        if (v.isCollectable()) {
            gc.markValue(v);
        }
    }

    // 标记协程的全局表
    if (state_->getGlobalTable()) {
        gc.markObject(state_->getGlobalTable());
    }

    // 标记协程的 open upvalues
    Upvalue* uv = state_->getOpenUpvalues();
    while (uv) {
        gc.markObject(uv);
        uv = uv->getNext();
    }
}

usize Thread::getMemorySize() const {
    return sizeof(Thread) + state_->getEstimatedMemorySize();
}
```

### 6.2 GC 注意事项

- Thread 生命周期由 GC 管理，**但** C++20 协程帧需要在 Thread 析构时显式 destroy：
  ```cpp
  Thread::~Thread() {
      if (coHandle_.handle && !coHandle_.handle.done()) {
          coHandle_.handle.destroy();  // 释放协程帧
      }
      delete state_;  // 释放协程的 LuaState
  }
  ```
- 协程帧内堆分配的资源由编译器管理，destroy 时自动释放
- 如果协程处于 suspended 状态被 GC 回收（无引用），协程帧会正确清理

---

## 7. 标准库函数实现

```cpp
// ═══════════════════════════════════════════
// 文件: src/lib/coroutinelib.cpp
// ═══════════════════════════════════════════

/// coroutine.create(f) → thread
static i32 coroutine_create(LuaState* L) {
    // 检查参数是 function
    if (!L->isFunction(-1)) {
        L->error("coroutine.create: argument must be a function");
    }
    Function* func = L->getStack().at(L->getAbsoluteTop() - 1).asFunction();

    // 创建 Thread 对象
    Thread* thread = Thread::create(L, func);

    // 将 Thread 压栈作为返回值
    L->pop(1);  // 弹出 function 参数
    L->pushValue(Value(thread));
    return 1;
}

/// coroutine.resume(co, ...) → ok, ...
static i32 coroutine_resume(LuaState* L) {
    i32 nargs = L->getTop() - 1;  // 除去 co 参数

    // 获取 thread 对象
    Value& coVal = L->getStack().at(L->getAbsoluteTop() - nargs - 1);
    if (!coVal.isThread()) {
        L->error("coroutine.resume: argument #1 must be a thread");
    }
    Thread* thread = coVal.asThread();

    // 执行 resume（内部处理值传递和状态管理）
    bool ok = thread->resume(L, nargs);

    // resume 内部已经将结果压入 L 的栈
    // 返回值数量 = 1(bool) + transfered results
    return L->getTop();  // resume 方法已清理并设置好栈
}

/// coroutine.yield(...) → ...
static i32 coroutine_yield(LuaState* L) {
    i32 nresults = L->getTop();  // 所有参数都是 yield 的返回值

    // 设置 yield 状态（VM 层会检测到并抛出 YieldSignal）
    Thread::yield(L, nresults);

    // 注意：正常情况下这里不会执行到
    // 因为 VM 层会抛出 YieldSignal 跳出
    // 恢复时，resume 参数会作为这个函数的"返回值"
    return 0;
}

/// coroutine.status(co) → string
static i32 coroutine_status(LuaState* L) {
    if (!L->isThread(-1)) {
        L->error("coroutine.status: argument must be a thread");
    }
    Thread* thread = L->getStack().at(L->getAbsoluteTop() - 1).asThread();

    const char* status = nullptr;
    switch (thread->getCoroutineStatus()) {
        case CoroutineStatus::Suspended: status = "suspended"; break;
        case CoroutineStatus::Running:   status = "running";   break;
        case CoroutineStatus::Normal:    status = "normal";    break;
        case CoroutineStatus::Dead:      status = "dead";      break;
    }

    L->pop(1);
    auto& pool = L->getGlobalState().getStringPool();
    L->pushString(pool.intern(status));
    return 1;
}

/// coroutine.wrap(f) → iterator function
static i32 coroutine_wrap(LuaState* L) {
    // 创建协程
    coroutine_create(L);  // 栈上: [thread]
    Thread* thread = L->getStack().at(L->getAbsoluteTop() - 1).asThread();

    // 创建一个 C 闭包，捕获 thread 作为 upvalue
    // 每次调用该闭包相当于执行 resume，但出错时直接 throw
    auto wrapIterator = [](LuaState* L) -> i32 {
        // 从 upvalue 获取 thread
        Thread* co = L->getUpvalue(1).asThread();
        i32 nargs = L->getTop();

        bool ok = co->resume(L, nargs);
        if (!ok) {
            // wrap 模式下出错直接抛异常
            L->error();  // 错误消息已在栈上
        }

        // 去掉 true 前缀，只返回 yield 的值
        return L->getTop() - 1;
    };

    // 创建带 upvalue 的 C 闭包
    Function* wrapper = Function::createCClosure(wrapIterator, 1);
    wrapper->setUpvalue(0, Value(thread));

    L->pop(1);  // 弹出 thread
    L->pushValue(Value(wrapper));
    return 1;
}

/// coroutine.running() → thread or nil
static i32 coroutine_running(LuaState* L) {
    Thread* running = L->getRunningThread();
    if (running) {
        L->pushValue(Value(running));
    } else {
        L->pushNil();
    }
    return 1;
}

/// 注册所有协程库函数
void openCoroutineLib(LuaState* L) {
    auto& pool = L->getGlobalState().getStringPool();
    Table* coroutineTable = new Table();

    struct FuncEntry { const char* name; CFunction func; };
    FuncEntry funcs[] = {
        { "create",  coroutine_create  },
        { "resume",  coroutine_resume  },
        { "yield",   coroutine_yield   },
        { "status",  coroutine_status  },
        { "wrap",    coroutine_wrap    },
        { "running", coroutine_running },
    };

    for (auto& entry : funcs) {
        GCString* name = pool.intern(entry.name);
        Function* func = new Function(entry.func);
        coroutineTable->rawSet(Value(name), Value(func));
    }

    GCString* libName = pool.intern("coroutine");
    L->getGlobalTable()->rawSet(Value(libName), Value(coroutineTable));
}
```

---

## 8. 实现步骤（阶段规划）

### Phase 1：基础设施（预计 200 行左右）

1. **创建 `Thread` 类骨架**（`thread.hpp/cpp`）
   - 继承 `GCObject`
   - 包含独立 `LuaState*`，状态字段
   - 暂不加 C++20 协程部分

2. **`LuaState::newThread()` 工厂方法**
   - 创建子线程，共享 `GlobalState`
   - 独立的 `Stack` + `CallStack`
   - 共享 `globalTable_`（或可选独立全局表）

3. **GC 集成**
   - `Thread::markChildren()` 标记栈上值
   - 在 `garbage_collector.cpp` 中处理 Thread 类型

4. **验证**：创建 Thread 对象，确认 GC 能正确标记和回收

### Phase 2：协程执行引擎（预计 300 行左右）

5. **定义 `YieldSignal` 和 `CoroutinePromise`**
   - 在 `vm.hpp` 或 `thread.hpp` 中

6. **实现 `coroutineBody()` C++20 协程函数**
   - 循环：执行 VM → catch YieldSignal → `co_await` → 恢复

7. **修改 VM CALL 路径**
   - 在 `vmPrecall` 返回后检查 `ThreadStatus::Yield`
   - 保存当前帧的 `savedpc`
   - 抛出 `YieldSignal`

8. **实现 `VM::resumeExecution()`**
   - 从 `L->getCurrentCallInfo().savedpc` 恢复执行
   - 与现有 `executeProto` 共享核心循环逻辑

9. **实现 `Thread::resume()` 和 `Thread::yield()`**
   - 值传递（栈间转移）
   - 状态管理

10. **验证**：手动构造 Thread，测试简单的 resume/yield 循环

### Phase 3：标准库函数（预计 200 行左右）

11. **实现 6 个 coroutine.* 函数**
12. **注册到 `lib_manager`**
13. **验证**：通过 Lua 脚本测试 `coroutine.create/resume/yield`

### Phase 4：边界和健壮性（预计 150 行左右）

14. **错误处理**：协程内错误 → `resume` 返回 `false, errmsg`
15. **嵌套协程**：协程 resume 其他协程时的 `normal` 状态
16. **`coroutine.wrap` 完整实现**（需要 C 闭包 + upvalue 支持）
17. **`coroutine.running` 完善**
18. **边界测试**：dead 协程 resume、重复 yield、栈溢出等

### Phase 5：单元测试（预计 200 行左右）

19. **基础测试**：create → resume → yield → resume → return
20. **值传递测试**：参数和返回值在 resume/yield 间正确传递
21. **多次 yield 测试**：生成器模式
22. **错误传播测试**：协程内 error → resume 返回 false
23. **状态测试**：suspended/running/normal/dead 正确转换
24. **wrap 测试**：迭代器模式
25. **GC 测试**：协程被回收时资源正确释放

---

## 9. 风险与应对

### 9.1 技术风险

| 风险 | 影响 | 应对策略 |
|------|------|---------|
| **YieldSignal 异常开销** | yield 路径有 exception 开销（~微秒级） | Lua 协程切换本就不是热路径；可用 benchmark 验证；若不可接受可改为 flag 检查 |
| **VM 恢复正确性** | 从保存的 PC 恢复时栈状态不一致 | 借鉴 pcall 的隔离模式；resumeExecution 需要正确恢复所有帧 |
| **嵌套协程的 CallInfo 链** | 协程 A resume 协程 B，B yield 时需要正确回到 A | 通过 `callerState_` 链追踪；状态机需仔细验证 |
| **C++20 协程帧与 GC 交互** | 协程帧内可能持有 GC 对象的裸指针 | 协程帧内不长期持有裸指针；所有 GC 对象通过 LuaState 栈访问 |
| **MSVC 协程 bug** | 早期 MSVC 有一些 C++20 协程 edge case | VS 2026 应已修复；保持关注编译器更新 |

### 9.2 YieldSignal 异常方案的替代

如果测试中发现 exception 开销不可接受，可以改为 **flag 检查 + 提前返回**：

```cpp
// 替代方案：所有 VM 执行路径增加 yield 检查
case OpCode::CALL: {
    // ... 执行 ...
    if (L->getStatus() == ThreadStatus::Yield) {
        ci.savedpc = &code[pc];
        return;  // 直接返回，不抛异常
    }
}
```

这要求 `executeProto` 的所有调用者都检查 Yield 状态，但避免了异常开销。可以在 Phase 2 实现后根据性能测试结果决定是否切换。

---

## 10. 与现有代码的兼容性矩阵

| 模块 | 影响度 | 改动说明 |
|------|--------|---------|
| `Value` | 🟢 无改动 | `Thread*` 变体已存在 |
| `GCObject` | 🟢 无改动 | Thread 继承 GCObject |
| `LuaState` | 🟡 小改动 | 增加 newThread()、yield 相关字段 |
| `VM::executeProto` | 🟡 小改动 | CALL 路径增加 yield 检测 |
| `vmPrecall/vmPostcall` | 🟢 无改动 | 逻辑不变 |
| `GarbageCollector` | 🟡 小改动 | markChildren 支持 Thread |
| `lib_manager` | 🟢 微改动 | 注册 coroutine 库 |
| `Stack` / `CallInfo` | 🟢 无改动 | 结构不变 |
| `pcall` | 🟢 无改动 | 独立机制 |
| 编译器 (Lexer/Parser/CodeGen) | 🟢 无改动 | 协程无需新字节码 |
| 其他标准库 | 🟢 无改动 | 独立模块 |

---

## 11. 预期的 Lua 测试用例

验收时，以下 Lua 脚本应能正确运行：

```lua
-- 基础 resume/yield
local co = coroutine.create(function(a, b)
    print("co start", a, b)       -- co start 1 2
    local c = coroutine.yield(a + b)
    print("co resume", c)         -- co resume 10
    return "done"
end)

print(coroutine.status(co))       -- suspended
local ok, val = coroutine.resume(co, 1, 2)
print(ok, val)                    -- true 3
print(coroutine.status(co))       -- suspended

ok, val = coroutine.resume(co, 10)
print(ok, val)                    -- true done
print(coroutine.status(co))       -- dead

-- 生成器模式
local gen = coroutine.wrap(function()
    for i = 1, 3 do
        coroutine.yield(i)
    end
end)

print(gen())  -- 1
print(gen())  -- 2
print(gen())  -- 3

-- 错误处理
local co2 = coroutine.create(function()
    error("oops")
end)
local ok2, err = coroutine.resume(co2)
print(ok2, err)  -- false  [error message containing "oops"]
```

---

## 12. 总结

本方案采用 **"Wrapper 协程 + YieldSignal"** 模式：

- **Thread** 类持有独立 `LuaState` + C++20 协程句柄
- **coroutineBody()** 作为 C++20 协程函数，循环执行 VM 并在 yield 时 `co_await` 挂起
- **VM 层** 通过检测 `ThreadStatus::Yield` 和抛出 `YieldSignal` 与 C++20 协程帧通信
- **值传递** 通过两个 `LuaState` 栈之间的值搬运实现

这种设计将改动集中在新增文件上，对现有 VM 的侵入最小，同时充分利用了 C++20 协程的自动帧管理和挂起/恢复机制。

> **等待评审**：请审阅上述方案，特别关注方案 A vs B 的选择、YieldSignal 异常机制、以及 GC 集成方案。
