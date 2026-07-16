---
status: current
verified_against: src/lua.h; src/api/lapi.cpp; src/runtime/execution_policy.hpp; src/runtime/sandbox_policy.hpp; src/runtime/runtime_services.hpp; src/vm/state/global_state.hpp; src/vm/state/global_state.cpp; src/vm/vm.cpp; src/vm/state/lua_state.cpp; src/core/thread.cpp; src/gc/gc_finalize.cpp; tests/unit/vm/test_runtime_services.cpp; tests/unit/gc/test_gc.cpp; tests/unit/api/test_lua_c_api.cpp; benchmarks/runtime_bench.cpp; tests/compatibility/runtime-benchmark-regression-policy.json
last_checked: 2026-07-16
applies_to: owner-thread access, instruction budget, monotonic deadline, external cancellation, cooperative native callback polling, per-drain finalizer budget, and context sandbox policy
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

VM 在每条 Lua 指令前检查取消、deadline、预算，优先级依次为 cancellation、deadline、instruction budget。C/C++ 函数本身若长期不返回，不会被 Lua 指令检查点抢占；宿主原生函数必须遵守下一节的协作式轮询合同。

## 原生回调协作式轮询

项目扩展 `lua_checkexecution(L)` 供长期运行的原生 C/C++ callback 在有界工作切片之间主动调用：

```c
static int do_work(lua_State* L) {
    while (has_more_work()) {
        lua_checkexecution(L);
        do_one_bounded_slice();
    }
    return 0;
}
```

该入口只轮询当前 context 的 atomic cancellation 和 monotonic deadline，不消费 Lua instruction budget，也不在成功时改动栈。若停止条件已满足，它像 `lua_error` 一样从可抛 C API 边界退出；外层 `lua_pcall` 返回 `LUA_ERRRUN` 并发布对应的预分配 fixed 错误对象。它是普通 owner-thread C API：外部线程仍只能调用预先取得的 `ExecutionCancellationHandle::requestCancellation()`，不得把 `lua_State*` 交给取消线程。

这是协作合同而非异步抢占。回调最大取消延迟由两个轮询点之间最慢工作切片决定；回调不得在两次轮询之间执行无界循环。阻塞系统调用、第三方库等待和 I/O 还必须使用各自的 timeout、可取消句柄或宿主中断机制；一个永久阻塞或从不调用 `lua_checkexecution` 的 callback 不受本策略强制终止。短小且确定有界的 callback 可以不增加额外轮询。

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

## 脚本能力策略

同一个 context 还拥有 `SandboxPolicy`。它在库打开时控制 base/math/io/string/table/os/coroutine/debug/package 的暴露，并在每次特权操作时控制文件系统、进程和原生模块能力。默认 unrestricted 保持 Lua 5.1 行为；game-server profile 只开放安全库和 preload-only package。详细能力矩阵、固定错误与信任边界见 [SandboxPolicy](sandbox-policy.md)。

## 性能与当前边界

默认策略仍经过一个 relaxed atomic cancellation load 和快速未启用分支。Release `vm_instructions_per_second` 已纳入 base-vs-head 相对回归策略，允许的最大退化比例为 20%。deadline 只有启用时才读取 `steady_clock`。

当前页闭环固定 owner-thread、instruction budget、monotonic deadline、atomic cancellation、原生 callback 协作式轮询、finalizer 单轮预算与 context sandbox/module policy。原生 C 函数仍不能被 VM 异步抢占，callback 必须遵守上述轮询与阻塞合同；不得把执行策略或脚本能力策略解释为 OS 级隔离。
