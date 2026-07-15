---
status: current
verified_against: src/runtime/execution_policy.hpp; src/runtime/runtime_services.hpp; src/vm/state/global_state.hpp; src/vm/state/global_state.cpp; src/vm/vm.cpp; src/vm/state/lua_state.cpp; src/core/thread.cpp; tests/unit/vm/test_runtime_services.cpp; benchmarks/runtime_bench.cpp; tests/compatibility/runtime-benchmark-regression-policy.json
last_checked: 2026-07-15
applies_to: instruction budget, monotonic deadline, and external cancellation
---

# ExecutionPolicy 执行治理合同

`ExecutionPolicy` 属于一个 `EngineContext` 的 `GlobalState`，不是某个单独 `LuaState` 的字段。主状态、子 coroutine、Lua→C→Lua 重入和 yield/resume 因而消费同一执行窗口；这些边界都不会隐式补充或重置预算。

## 配置与所有权

宿主 owner thread 在没有 Lua 代码执行时调用 `EngineContext::executionPolicy().configure(limits)`。`Limits` 包含：

- `instructionBudget`：值为 `N` 时恰好允许执行 `N` 条 Lua VM 指令；默认 `UnlimitedInstructions`。
- `deadline`：`std::chrono::steady_clock::time_point`，默认 `time_point::max()`。它不受系统墙钟或时区调整影响。

`configure` 同时清除旧 cancellation；`reset` 恢复无限预算、无 deadline、未取消。配置字段不是并发可写的，宿主不得在 VM 正运行时调用这两个函数。

## 唯一跨线程入口

`EngineContext::cancellationHandle()` 返回非 owning 的 `ExecutionCancellationHandle`。外部线程只能调用 `requestCancellation()`，它以原子标志发出单向请求，不暴露 `LuaState`、栈、GC 或策略配置。handle 不得超过其 `EngineContext` 生命周期。

VM 在每条 Lua 指令前检查取消、deadline、预算，优先级依次为 cancellation、deadline、instruction budget。C/C++ 函数本身若长期不返回，不会被 Lua 指令检查点抢占；宿主原生函数仍须自行遵守阻塞与取消合同。

## 错误与保护调用

三种停止原因分别使用 context 初始化时预分配并 fixed 的字符串：

- `execution cancelled`
- `execution deadline exceeded`
- `execution instruction budget exceeded`

VM 抛出携带该 Lua `Value` 的 `RuntimeError`。`pcall`/`lua_pcall` 返回 `LUA_ERRRUN` 并保留同一对象，不需要在故障路径再次分配字符串；coroutine resume 返回 `false, error`。错误不会自动 reset 策略，owner 必须显式配置下一执行窗口。

## 性能与当前边界

默认策略仍经过一个 relaxed atomic cancellation load 和快速未启用分支。Release `vm_instructions_per_second` 已纳入 base-vs-head 相对回归策略，允许的最大退化比例为 20%。deadline 只有启用时才读取 `steady_clock`。

当前页只闭环 instruction budget、monotonic deadline 与 atomic cancellation。assessment 路线中的 finalizer 单轮预算、sandbox/module policy、owner-thread 运行时断言和原生 C 函数协作式取消由 [#10](https://github.com/YanqingXu/lua/issues/10) 跟踪；不得将本阶段解释为完整 server sandbox。
