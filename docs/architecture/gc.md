---
status: current
verified_against: src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/gc/gc_strategy.hpp; src/gc/gc_strategy.cpp; src/gc/gc_mark.cpp; src/gc/gc_sweep.cpp; src/gc/gc_finalize.cpp; src/core/string_pool.cpp; src/core/gc_object.hpp; src/core/table.cpp; src/core/function.cpp; src/core/upvalue.cpp; src/core/userdata.cpp; src/core/thread.cpp; src/vm/state/global_state.cpp; tests/unit/gc/test_gc.cpp
last_checked: 2026-05-31
applies_to: current garbage collector implementation
---

# 垃圾回收（Garbage Collection）

当前回收器是基于 GlobalState 的回收器，通过 `RuntimeServices::gc`、`EngineContext::gc()` 和 `GlobalState::getGC()` 暴露。回收通过 `GCStrategy` 边界运行。默认的 `MarkSweepGC` 策略实现了真正的 stop-the-world 标记-清除算法；`collectgarbage("step")` 使用内部的分阶段调度器，而 `IncrementalGC` 对于完整 `collect()` 调用仍为教学占位实现。旧有的 `GarbageCollector::getInstance()` 仍作为已弃用的兼容垫片存在。

## 托管对象

每个可回收对象继承自 `GCObject` 并实现：

- `mark(GarbageCollector&)`
- `getSize()`

当前托管类型包括 `GCString`、`Table`、`Proto`、`Function`、`Upvalue`、`Userdata` 和 `Thread`。

对象通过回收器的 `allObjects_` 链表链接。大多数对象构造函数不会自动注册自身；创建点在对象应加入 GC 所有权时调用 `registerObject()`。`GCString` 对象由 `StringPool` 注册。

## 回收流程

`GarbageCollector::collect(LuaState* currentState)` 创建 `GCContext` 并委托给活跃的 `GCStrategy`。当前标记-清除实现按以下步骤运行：

1. 清除瞬时标记状态并标记根。
2. 标记显式根和 `GlobalState` 根。
3. 通过调用每个对象的 `mark()` 传播灰色对象。
4. 准备携有 `__gc` 终结器的不可达 userdata。
5. 在清除前清理弱表条目。
6. 使用显式 `StringPool&` 清除不可达对象，以便从同一驻留表中移除已死的 `GCString` 条目。
7. 在 `LuaState` 可用时运行排队的终结器。

`collectgarbage("collect")` 通过基础库进入此路径。

## 策略边界

`src/gc/gc_strategy.hpp` 定义了：

- `GCContext`：回收器、显式 `StringPool&` 和可选的当前 `LuaState`
- `GCStrategy`：抽象回收策略接口
- `MarkSweepGC`：默认实现
- `IncrementalGC`：对完整 `collect()` 调用保持等价可达性行为的占位策略

活跃策略可通过 `GarbageCollector::getStrategyName()` 查询。`collectgarbage("strategy")` 返回当前策略名称，`collectgarbage("strategy", "mark-sweep" | "incremental")` 切换活跃边界。增量策略刻意保持保守：它对完整 `collect()` 调用复用标记-清除回收，而 `collectgarbage("step")` 演练回收器的分阶段 pause/propagate/atomic/sweep/finalize 路径。

`collectgarbage("setpause", n)` 和 `collectgarbage("setstepmul", n)` 现在存储真正的回收器参数并返回先前值。`pause` 影响自动回收后的自动 GC 阈值；`stepmul` 缩放 `step` 工作预算。这些控制参数是当前回收器的兼容面，但工作量核算仍是项目本地的近似，而非 Lua 5.1 的逐字节债务模型。

## 增量步进流程

`collectgarbage("step")` 驱动以下状态：

1. `pause`：等待分配债务达到下一个阈值。
2. `propagate`：每步扫描有限数量的灰色对象。
3. `atomic`：重新扫描根，处理弱表，准备终结器，关闭标记。
4. `sweep`：按有限片清除对象链表，并从所属 `StringPool` 中移除已死的驻留字符串。
5. `finalize`：在任意 VM 分配点之外运行排队的 userdata 终结器。

关键不变式：

- 黑色对象不得指向在同一周期中可被清除的白色对象
- 弱键和弱值在标记后、对象删除前被清理
- 可终结的 userdata 复活一个周期，再次标记，最多终结一次
- 从终结器中复活的 userdata 和可到达对象在当前周期中存活
- 根包括注册表、基础类型元表、元方法名称、内存错误字符串、当前/主线程栈、运行中的协程、debug hook、open upvalue 和待处理终结器

当前实现状态：

- `collect()` 仍是通过活跃 `GCStrategy` 的完整标记-清除周期
- `step()` 开始标记、按预算传播灰色对象、执行原子弱表/终结器准备、按游标清除、在周期结束时运行终结器
- 保守写屏障为当前变更点保护三色不变式
- `setpause` / `setstepmul` 是有状态的，影响自动阈值 / step 预算
- debug-hook 触发的手动回收保护被打断的 Lua 寄存器窗口，而不会保持普通已死临时对象存活

## 写屏障

`GarbageCollector::writeBarrier(owner, child)` 及其 `Value` 重载实现了保守的前向屏障。当黑色 owner 接收到同一回收器拥有的白色 child 时，child 被标记并立即传播其图。这比 Lua 5.1 的增量回收器更急切，但在阶段游标尚未完成时保护了相同的正确性不变式。

覆盖的变更点：

- `Table::set`、`Table::setArray` 和 `Table::setMetatable`
- `Userdata::setMetatable`
- `Function::setEnv`、`Function::setUpvalue` 和 `Function::addUpvalue`
- `Upvalue::setValue` 和 `Upvalue::close`
- `GlobalState::setMetatable` 和 `GlobalState::setRunningThread`（通过根屏障）

`tests/unit/gc/test_gc.cpp` 包含回归探针，使 owner 变黑，通过这些路径附加白色 child 图，然后清除以确保新引用被保留。

## 根集

根集包括：

- 通过 `addRoot` 注册的显式根
- 注册表
- 内存错误字符串
- 固定元方法名称和保留字符串
- 基础类型元表
- 当前状态栈和调用帧
- 主线程栈和调用帧
- 运行中的协程线程
- debug hook 函数
- 待处理终结器 userdata

`GlobalState::markRoots()` 协调共享运行时根。

## 弱表

`Table` 通过元表字段 `__mode` 支持弱键和弱值：

- `"k"`：弱键
- `"v"`：弱值
- `"kv"`：弱键和弱值

在标记期间，`GarbageCollector::markTable()` 记录弱表并避免标记弱侧。在清除前，`clearWeakTableEntries()` 移除弱键或弱值即将死亡的条目。

## Userdata 终结器

`Userdata` 可以在其元表中拥有 `__gc` 终结器。带有终结器的不可达 userdata 复活一个回收周期、排队，并通过 `runFinalizers()` 终结。

当前行为：

- 终结器接收 userdata 作为参数
- 同一终结器不应在同一 userdata 上调用两次
- 终结器错误被包容，因此单个失败的终结器不会中断整个回收周期

## 已知限制

- `IncrementalGC` 不改变完整 `collect()` 行为；它在策略边界后保留标记-清除语义。
- `collectgarbage("step")` 有分阶段边界，但其工作单元是项目本地的近似，而非 Lua 5.1 的精确债务核算。
- 已弃用的旧版回收器单例保留用于兼容。
- `clearAll()` 保留固定对象；这匹配当前测试/关闭行为，但不是通用的堆拆卸 API。

## 验证

```powershell
bin\lua_test.exe --filter "GC"
bin\lua_test.exe --filter "collectgarbage"
```
