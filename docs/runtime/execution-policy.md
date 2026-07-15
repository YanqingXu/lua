---
status: current
verified_against: src/runtime/execution_policy.hpp; src/runtime/runtime_services.hpp; src/vm/state/global_state.hpp; src/vm/state/global_state.cpp; src/vm/vm.cpp; src/vm/state/lua_state.cpp; src/core/thread.cpp; src/gc/gc_finalize.cpp; tests/unit/vm/test_runtime_services.cpp; tests/unit/gc/test_gc.cpp; tests/unit/api/test_lua_c_api.cpp; benchmarks/runtime_bench.cpp; tests/compatibility/runtime-benchmark-regression-policy.json
last_checked: 2026-07-15
applies_to: owner-thread access, instruction budget, monotonic deadline, external cancellation, and per-drain finalizer budget
---

# ExecutionPolicy 执行治理合同

`ExecutionPolicy` 属于一个 `EngineContext` 的 `GlobalState`，不是某个单独 `LuaState` 的字段。主状态、子 coroutine、Lua→C→Lua 重入和 yield/resume 因而消费同一执行窗口；这些边界都不会隐式补充或重置预算。

## 配置与所有权

宿主 owner thread 在没有 Lua 代码执行时调用 `EngineContext::executionPolicy().configure(limits)`。`Limits` 包含：

- `instructionBudget`：值为 `N` 时恰好允许执行 `N` 条 Lua VM 指令；默认 `UnlimitedInstructions`。
- `deadline`：`std::chrono::steady_clock::time_point`，默认 `time_point::max()`。它不受系统墙钟或时区调整影响。
- `finalizerBudgetPerDrain`：一次完整 GC、增量 finalize 阶段或 `lua_close` drain 最多进入的用户 `__gc` 回调数；默认 `UnlimitedFinalizers`，因此不改变 Lua 5.1 默认行为。值为 `0` 时该 drain 不进入任何用户 finalizer。

`configure` 同时清除旧 cancellation；`reset` 恢复无限 instruction/finalizer 预算、无 deadline、未取消。配置字段不是并发可写的，宿主不得在 VM 正运行时调用这两个函数。

## 固定 owner thread

创建 `EngineContext`/`GlobalState` 的线程是不可变 owner；当前没有 transfer 或 rebind API。`EngineContext` 服务访问、`RuntimeServices` 构造、VM 入口及公开 C API 都在接触可变 runtime 状态前验证该身份。可抛入口以 `RuntimeOwnerThreadError` 报告宿主逻辑错误；protected/`noexcept` 入口返回各自失败值并保持栈不变。foreign-thread `lua_close` 不执行销毁，必须由 owner 重试；owning context 若在 foreign thread 析构则 `std::terminate`。

## 唯一跨线程入口

`EngineContext::cancellationHandle()` 返回非 owning 的 `ExecutionCancellationHandle`。外部线程只能调用 `requestCancellation()`，它以原子标志发出单向请求，不暴露 `LuaState`、栈、GC 或策略配置。handle 不得超过其 `EngineContext` 生命周期。

VM 在每条 Lua 指令前检查取消、deadline、预算，优先级依次为 cancellation、deadline、instruction budget。C/C++ 函数本身若长期不返回，不会被 Lua 指令检查点抢占；宿主原生函数仍须自行遵守阻塞与取消合同。

## 错误与保护调用

三种停止原因分别使用 context 初始化时预分配并 fixed 的字符串：

- `execution cancelled`
- `execution deadline exceeded`
- `execution instruction budget exceeded`

VM 抛出携带该 Lua `Value` 的 `RuntimeError`。`pcall`/`lua_pcall` 返回 `LUA_ERRRUN` 并保留同一对象，不需要在故障路径再次分配字符串；coroutine resume 返回 `false, error`。错误不会自动 reset 策略，owner 必须显式配置下一执行窗口。

## Finalizer 单轮预算

finalizer 预算按 drain 重新补充，不像 instruction budget 那样跨执行窗口累计消费。有限预算对以下路径使用同一上限：

- 完整 `collect()` 和增量周期的 finalize 阶段只进入该数量的用户回调；未消费的 userdata 保持在 GC rooted pending queue 中，由后续收集继续处理。
- finalizer 中的重入收集受 `finalizersRunning` 防护，不能绕过外层 drain 的上限；每个 userdata 仍最多终结一次。
- `lua_close` 达到上限后不再进入其他用户回调，但仍销毁所有 GC 对象和 allocator-backed 存储，保持关闭路径 `noexcept` 且 allocator 计数归零。

该字段限制的是回调进入次数，不是时间配额。Lua 实现的 `__gc` 同时受 instruction budget、deadline 和 cancellation 检查；长期不返回的原生 C/C++ finalizer 仍须由宿主实现协作式取消。

## 性能与当前边界

默认策略仍经过一个 relaxed atomic cancellation load 和快速未启用分支。Release `vm_instructions_per_second` 已纳入 base-vs-head 相对回归策略，允许的最大退化比例为 20%。deadline 只有启用时才读取 `steady_clock`。

当前页闭环固定 owner-thread、instruction budget、monotonic deadline、atomic cancellation 与 finalizer 单轮预算。assessment 路线中的 sandbox/module policy 和原生 C 函数协作式取消继续由 [#10](https://github.com/YanqingXu/lua/issues/10) 跟踪；不得将本阶段解释为完整 server sandbox。
