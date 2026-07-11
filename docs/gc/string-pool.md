---
status: current
verified_against: src/core/string_pool.hpp; src/core/string_pool.cpp; src/core/gc_string.hpp; src/core/gc_string.cpp; src/gc/garbage_collector.cpp; src/gc/gc_sweep.cpp; tests/unit/gc/; tests/unit/core/
last_checked: 2026-07-11
applies_to: StringPool 驻留、GC 注册与清除不变量
---

# StringPool 驻留、GC 注册与清除不变量

StringPool 将相同字节序列规范化为同一 `GCString*`，并与所属 GarbageCollector 协作管理生命周期。pool 是驻留索引，不是让所有历史字符串永久存活的强 root。

## 驻留路径

```text
bytes
  → lookup hash/content
  ├── hit  → existing GCString*
  └── miss → allocate GCString
             → register with collector
             → insert intern entry
```

相同 pool 内的相同内容返回同一对象，因此内部字符串比较和 table key 快路径可以利用身份。不同 `EngineContext` 拥有不同 pool，相同文本不要求跨上下文共享地址。

## 与 GC 的关系

GCString 与其他 GCObject 一样只在从 roots/强边可达时存活。sweep 回收字符串前必须从创建它的 StringPool 删除驻留项；否则后续 lookup 会返回悬空指针。

完整 collection 的 `GCContext` 显式携带 `StringPool&`，使 collector 不依赖隐藏全局 pool。独立测试 collector 的兼容回退也必须保证注册与清除使用同一实例。

## 边界与不变量

- pool key 的 hash/equality 按字节内容，不依赖临时 `StrView` 生命周期。
- GCString 注册成功后才能作为普通 Value 暴露。
- sweep 删除对象与 pool entry 是同一生命周期事务。
- 固定/保留字符串由 GlobalState roots 保持可达，而不是使用另一套 delete 规则。
- pool 析构或运行时关闭不会重复释放已由 collector 拥有的对象。
- 字符串展示、number conversion 与驻留是独立职责。

这类设计展示了现代 C++ 所有权边界：StringPool 拥有索引结构，GarbageCollector 拥有对象释放权，Value 只观察 GCString；三者不能都被描述为对象 owner。
