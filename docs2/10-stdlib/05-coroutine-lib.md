# Coroutine Library — 协程库

## 6/6 函数全部实现

| 函数 | 说明 |
|------|------|
| `coroutine.create(f)` | 创建协程 |
| `coroutine.resume(co, ...)` | 恢复/启动协程 |
| `coroutine.yield(...)` | 挂起并返回 |
| `coroutine.status(co)` | 状态: "running"/"suspended"/"dead"/"normal" |
| `coroutine.running()` | 返回当前协程 |
| `coroutine.wrap(f)` | 创建函数形式的协程 (无显式 resume) |

## 实现

每个协程有独立的 LuaState + Stack。
通过 `Thread` 对象管理，继承自 GCObject。
