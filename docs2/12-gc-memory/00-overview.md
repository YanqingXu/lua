# GC & Memory Overview — 垃圾回收与内存

## 1. GC 架构

```
GarbageCollector (GC 主控制器)
  ├── GCStrategy (策略接口)
  │     ├── MarkSweepGC (完整标记-清除)
  │     └── IncrementalGC (教学占位)
  ├── gc_mark.cpp   (标记阶段)
  ├── gc_sweep.cpp  (清除阶段)
  ├── gc_finalize.cpp (终结器)
  └── gc_weak.cpp   (弱表)

所有 GC 对象:
  ├── GCString
  ├── Table
  ├── Function
  ├── Userdata
  ├── Thread
  ├── Proto (内部)
  └── Upval (内部)

根集:
  ├── GlobalState.registry
  ├── LuaState 栈 (所有活跃值)
  ├── Open Upvalue 链表
  ├── GlobalState.metatables
  └── MainThread
```

## 2. 标记-清除流程

```
1. Mark Phase (标记 - Gray)
   - 根集标记为 Gray
   - 遍历 Gray 对象 → markChildren → 子对象标记为 Gray
   - 自身标记为 Black
   - 重复直到没有 Gray

2. Sweep Phase (清除)
   - 遍历所有对象
   - White → 回收
   - Black → 重置为 White (准备下一轮)

3. Finalize Phase (终结)
   - 对带有 __gc 元方法的 White 对象调用 finalizer
   - 两阶段: resurrect → mark again → finalize
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/gc/garbage_collector.cpp` | GC 入口和调度 |
| `src/gc/gc_strategy.cpp` | 策略接口 |
| `src/gc/gc_mark.cpp` | 标记 |
| `src/gc/gc_sweep.cpp` | 清除 |
| `src/gc/gc_finalize.cpp` | 终结器 (__gc) |
| `src/gc/gc_weak.cpp` | 弱表 |
| `src/core/gc_object.cpp` | GCObject 基类 |

## 4. GC 控制

```lua
collectgarbage("collect")   -- 强制执行完整 GC
collectgarbage("stop")      -- 停止自动 GC
collectgarbage("restart")   -- 恢复自动 GC
collectgarbage("step", n)   -- 执行 n 步 GC
collectgarbage("count")     -- 返回内存使用 (KB)
collectgarbage("setpause", n)  -- 设置 GC pause
collectgarbage("setstepmul", n) -- 设置 step multiplier
```
