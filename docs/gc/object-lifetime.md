# Object Lifetime — 对象生命周期

## 1. GC 对象生命周期

```
创建 (new / create)
  ├── 添加到 GC 链表
  ├── 初始颜色: White
  └── 开始使用

使用中
  ├── 被 Value 引用 (栈 / table / closure)
  ├── 被标记时: White → Gray → Black
  └── 被修改时: Black → Gray (写屏障)

不再被引用
  ├── 栈弹出 / table 删除 / closure 释放
  └── 对象仍在 GC 链表中 (White)

GC 周期
  ├── Mark: 标记所有可达对象
  ├── Sweep: 回收 White 对象
  │     ├── 有 __gc? → Resurrect → Finalize
  │     └── 否则 → free()
  └── 从 GC 链表移除
```

## 2. 各类型对象的生命周期

| 类型 | 创建 | 持有者 | 回收条件 |
|------|------|--------|---------|
| **GCString** | StringPool::intern() | Value (栈/表) | 无 Value 引用 |
| **Table** | new Table() | Value | 无 Value 引用 |
| **Function** | Closure 包装 | Value | 无 Value 引用 |
| **Proto** | CodeGen::generate() | Function / subProto | 无 Function 引用 |
| **Upval** | findOrCreateUpvalue() | Closure / LuaState | 无 Closure 引用 |
| **Userdata** | Userdata::create() | Value | 无 Value 引用 |
| **Thread** | LuaState 创建 | Value / GlobalState | 无引用 |

## 3. 特殊生命周期

- **StringPool 中的字符串**: 只要还在 pool 中就不会被回收 (因为 pool 持有强引用)
- **Proto**: 可能被多个 Closure 共享，只有所有 Closure 都不引用时才回收
- **Upvalue**: Open 状态下被 LuaState 持有，Closed 后被 Closure 持有
