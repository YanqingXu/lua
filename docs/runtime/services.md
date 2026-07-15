---
status: current
verified_against: src/runtime/runtime_services.hpp; src/runtime/execution_policy.hpp; src/runtime/native_module_registry.hpp; src/runtime/native_module_registry.cpp; src/core/string_pool.hpp; src/vm/state/global_state.hpp; src/vm/state/global_state.cpp; src/vm/state/lua_state.hpp; src/vm/state/lua_state.cpp; src/gc/garbage_collector.hpp; src/gc/gc_strategy.hpp; src/gc/gc_sweep.cpp; src/vm/vm_dispatch_strategy.hpp; src/vm/vm.cpp; src/main.cpp; src/repl.cpp; src/bytecode/bytecode_main.cpp; src/compiler/parser/parser.hpp; src/compiler/codegen/codegen.hpp; src/vm/vm.hpp; tests/compatibility/public_native_module.c; tests/compatibility/public_native_module_host.cpp; tests/unit/vm/test_runtime_services.cpp; tests/unit/vm/test_vm_dispatch.cpp; tests/unit/gc/test_gc.cpp
last_checked: 2026-07-15
applies_to: current RuntimeServices boundary
---

# 运行时服务（Runtime Services）

`RuntimeServices` 是一个小型显式依赖束：

```cpp
struct RuntimeServices {
    GlobalState& globalState;
    StringPool& strings;
    GarbageCollector& gc;
    VM::DispatchStrategy* dispatchStrategy;
};
```

它刻意保持轻薄。主要可执行路径现在创建 owning `EngineContext`，而 `GlobalState::getInstance()` 和旧有的 `GarbageCollector::getInstance()` 保留为旧式重载和测试的兼容垫片。新增的编译器、VM、标准库和 GC 路径接收它们使用的服务，而非在每个调用点访问单例。VM 执行可接收可选的分发策略用于测试或教学 switch/table 分发差异；GC 执行通过回收器访问活跃的 `GCStrategy`。

## 为什么存在

项目正在从隐式全局访问向显式运行时边界迁移。`RuntimeServices` 为此迁移提供了窄第一步：

- parser/codegen 入口点可共享同一字符串池
- VM 执行可以是上下文感知的
- VM 分发可使用默认的 `SwitchDispatch` 或注入的 `TableDispatch`
- CLI 工具可创建一个服务束并传递它
- GC 清除在移除驻留字符串时显式接收相关的 `StringPool&`
- GC 策略选择保持在 `RuntimeServices.gc` 之后
- 测试可断言边界而无需进行大规模所有权重写

## 当前使用者

| 使用者 | 路径 |
|---|---|
| 解释器应用 | `src/main.cpp` |
| REPL | `src/repl.cpp` |
| 字节码工具 | `src/bytecode/bytecode_main.cpp` |
| Parser 重载 | `src/compiler/parser/parser.hpp`, `src/compiler/parser/parser.cpp` |
| CodeGenerator 构造函数 | `src/compiler/codegen/codegen.hpp`, `src/compiler/codegen/codegen.cpp` |
| VM execute 重载 | `src/vm/vm.hpp`, `src/vm/vm.cpp`, `src/vm/vm_entry.cpp` |
| 标准库调用点 | `src/lib/baselib.cpp`, `src/lib/debuglib.cpp`, `src/lib/packagelib.cpp`, `src/lib/stringlib.cpp`, `src/lib/tablelib.cpp` |
| 测试 | `tests/unit/vm/test_runtime_services.cpp` |

## 当前契约

- `RuntimeServices::fromSingletons()` 是兼容构造函数。
- `RuntimeServices(GlobalState&, VM::DispatchStrategy* = nullptr)` 从该全局状态派生字符串池和回收器。
- `RuntimeServices(GlobalState&, StringPool&, GarbageCollector&, VM::DispatchStrategy* = nullptr)` 用于显式装配。
- `dispatchStrategy == nullptr` 表示 VM 执行使用 `VM::defaultDispatchStrategy()`。
- 它不拥有服务，其生命周期不得超过引用的运行时对象。

## Owning EngineContext

`EngineContext` 是 `RuntimeServices` 的所有权对应物：

```cpp
class EngineContext {
public:
    RuntimeServices services(VM::DispatchStrategy* dispatch = nullptr) noexcept;
    GlobalState& globalState() noexcept;
    StringPool& strings() noexcept;
    GarbageCollector& gc() noexcept;
    NativeModuleRegistry& nativeModules() noexcept;
    ExecutionPolicy& executionPolicy() noexcept;
    ExecutionCancellationHandle cancellationHandle() noexcept;
};
```

每个上下文拥有：

- 独立的 `StringPool`
- 独立的 `GlobalState`
- 由该全局状态拥有的 `GarbageCollector`
- 由该全局状态拥有的 `NativeModuleRegistry`
- 由该全局状态拥有、供所有 LuaState/coroutine 共享的 `ExecutionPolicy`
- 在该全局状态内创建的注册表、基础类型元表、保留字符串、元方法名称和主线程簿记

`LuaState::newState(EngineContext&)` 在该上下文内创建主状态。测试断言两个上下文将相同文本驻留为不同的 `GCString` 对象，且这些字符串属于不同的回收器。

执行预算、单调 deadline 和跨线程取消的详细所有权与错误合同见 [ExecutionPolicy](execution-policy.md)。

## 原生模块生命周期

`package.loadlib` 不再使用进程级静态 handle map，而是通过当前 `GlobalState::getNativeModules()` 获取 context-owned registry：

- 同一 context 内按规范化完整路径只取得一个 OS lease；
- 不同 context 分别取得 lease，因此一个 context 关闭不会使另一个 context 的函数指针失效；
- registry 在 `GlobalState` 中声明于 GC 之前，按 C++ 逆序析构规则晚于 GC 销毁，保证所有可能保存模块函数指针的 `Function` 先释放；
- 最后一个 context 释放 lease 后才卸载模块；Windows 当前进程的 `GetModuleHandle` 是 borrowed handle，不调用 `FreeLibrary`；
- POSIX 使用 `RTLD_LOCAL`，不把模块符号扩散到全局解析空间。

当前策略没有 eager unload 或 hot reload：只要 context 存活，已取得的 handle 就保持有效。独立纯 C fixture 只包含公开 `lua.h`，分别由 `lua_app`、纯公开 API embedding executable 和双-context 生命周期 host 加载；测试覆盖 missing file/open、missing symbol/init、一个 context 关闭后另一个继续调用，以及最后一个关闭后的实际卸载/重新加载。

隔离的是 registry cache、OS lease、Lua registry state 和关闭路径，不是动态库的 C static storage。两个并存 context 通常会由 OS loader 映射到同一模块实例，因此进程静态变量仍可共享；需要完全隔离的模块状态必须存入 `lua_State` registry 或宿主 context。模块 cache 元数据和 OS loader 分配也不计入 Lua heap hard-limit 声明，详见 [内存合同](memory-contract.md)。

## 隔离与兼容边界

`EngineContext` 现已被 `lua_app` 和 `lua_bytecode` 可执行入口点使用。一些 VM/编译器兼容重载和旧测试仍有意使用 `RuntimeServices::fromSingletons()`。对于新的隔离测试和新的嵌入面，优先使用 `EngineContext`；仅在调用者契约要求保留旧行为时才使用 `fromSingletons()`。

## 已文档化的单例例外

当前 `src/` 单例引用是有意的兼容或空上下文回退：

| 路径 | 例外说明 |
|---|---|
| `src/runtime/runtime_services.hpp` | 定义 `RuntimeServices::fromSingletons()` 作为旧式构造函数。 |
| `src/vm/state/global_state.*` | 拥有已弃用的单例访问器和默认构造函数垫片。 |
| `src/gc/garbage_collector.cpp` | 拥有已弃用的旧式 GC 访问器和用于仅回收器测试的独立字符串池回退。 |
| `src/vm/state/lua_state.cpp` | 无参 `LuaState::create()` / `newState()` 保留为旧式单例垫片；`newState(EngineContext&)` 是隔离路径。 |
| `src/vm/vm*.cpp` | 无服务的 VM 重载保留旧调用点；接收服务的重载是现代路径。 |
| `src/compiler/codegen/codegen.cpp` | `StringPool*` 构造函数保持旧测试和工具源码兼容；接收服务的构造函数是首选。 |
| `src/core/metatable.cpp` | 无 `LuaState*` 的重载回退到单例全局元表用于旧式辅助函数。 |
| `src/gc/gc_finalize.cpp`, `src/gc/gc_weak.cpp` | 没有 owning `GlobalState` 的回收器实例仅在旧式/测试回收器中使用单例。 |

新的生产或隔离测试应优先使用 `EngineContext`，避免扩大兼容单例的依赖面。official suite 的 C++ 脚手架为每个门禁创建独立上下文，因此用例隔离不依赖清理全局 singleton。
