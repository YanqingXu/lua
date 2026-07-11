---
status: current
verified_against: src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/gc/gc_strategy.hpp; src/gc/gc_mark.cpp; src/gc/gc_sweep.cpp; src/gc/gc_finalize.cpp; src/gc/gc_weak.cpp; src/core/gc_object.hpp; src/core/string_pool.cpp; tests/unit/gc/test_gc.cpp; src/gc/; src/core/string_pool.hpp; tests/unit/gc/
last_checked: 2026-07-11
applies_to: GC 对象图、生命周期、阶段与诊断入口
---

# GC 对象图、生命周期、阶段与诊断入口

项目 GC 以 `GlobalState` 为运行时边界，通过 `GarbageCollector` 跟踪 `GCObject` 图。默认策略是标记清除；`step()` 在同一正确性不变量下分阶段推进。本文只建立模型与阅读入口，具体算法集中在 [GC 实现](implementation.md)。

## 对象生命周期

```text
construct
  → registerObject / intern string
  → reachable through roots or GC edges
  → mark/propagate
  → unreachable white
     ├── ordinary object → sweep
     └── finalizable userdata → queue/resurrect → finalize → later sweep
```

托管类型包括 GCString、Table、Proto、Function、Upvalue、Userdata 和 Thread。C++ 裸指针表达 GC 图中的 observer edge，不表达 delete 权；对象释放只由所属 GarbageCollector 完成。

## Roots 与边

roots 包括显式 root、registry、类型 metatable、固定字符串、主/当前线程、活动协程、Lua 栈/调用帧、debug hook、open upvalue 和待终结对象。

强边由对象的 `mark()` 暴露。table 的弱 key/value 在 mark 后、sweep 前专门清理。黑对象在增量周期中接入白对象时必须经过写屏障，保持三色不变量。

## 阶段

| 阶段 | 工作 | 关键不变量 |
|---|---|---|
| pause | 等待债务/阈值 | 不改变图颜色语义 |
| propagate | 按预算扫描灰对象 | 扫描后的对象变黑，子边已处理 |
| atomic | 重扫 roots、处理弱边、准备 finalizer | 关闭本周期可达性结论 |
| sweep | 释放白对象、重置存活颜色 | string pool 与对象链同步移除 |
| finalize | 在安全 VM 边界调用 `__gc` | 每个 userdata 最多终结一次，错误隔离 |

完整 `collect()` 通过策略边界执行完整周期；step 的工作单位是项目本地近似，不承诺复制 Lua 5.1 的逐字节债务算法。

## String pool 与弱表

StringPool 同时是驻留索引和 GC 协作者。回收 GCString 时必须从同一 pool 移除条目，否则规范指针会悬空。详见 [字符串池](string-pool.md)。

弱表不会通过弱侧保持对象存活。finalizable userdata、ephemeron-like 关系和跨周期观察需要按实际阶段验证，详见 [弱表](weak-table.md) 与 [完整周期 walkthrough](cycle-walkthrough.md)。

## 诊断顺序

内存增长或悬空问题按以下证据收敛：

1. 对象是否注册到正确 collector；
2. 预期 root/强边是否被 `mark()` 遍历；
3. graph mutation 是否经过 barrier；
4. 弱边是否在 atomic 后清理；
5. finalizer 是否导致对象延迟一个周期或复活；
6. sweep 是否同步更新 StringPool 和对象链。

测试应观察对象可达性、弱条目、finalizer 次数和跨周期结果，不绑定地址、容器遍历顺序或某 step 精确处理几个对象。

## C++23 教学边界

RAII 管理 collector 自身、root guard、文件句柄和临时注册；Lua 对象图使用 tracing GC。把所有边替换成 `shared_ptr` 会改变循环、弱引用、finalizer 时机和语言语义。清晰区分 owner（collector）、root、strong GC edge、weak edge 与临时 observer，才是这里真正的现代 C++ 价值。
