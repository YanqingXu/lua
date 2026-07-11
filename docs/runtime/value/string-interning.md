# String Interning — 字符串驻留

## 1. 目标

`StringPool` 保证同一运行时上下文中，相同字节内容对应同一个 `GCString*`。这既减少重复存储，也让相等字符串可以走指针快速路径。

## 2. 所有权

| 文件 | 作用 |
|---|---|
| `src/core/string_pool.hpp/.cpp` | 驻留表、创建、查找和删除 |
| `src/core/gc_string.hpp/.cpp` | GC 管理的字符串对象 |
| `src/runtime/runtime_services.hpp` | 将字符串池作为显式运行时服务传递 |

`EngineContext` 拥有可隔离的 `StringPool`。`StringPool::getInstance()` 仍是兼容入口，不是新代码唯一可用的所有权模型。

## 3. 驻留流程

```text
intern(bytes)
  -> 在 unordered_map 中按内容查找
  -> 命中：返回已有 GCString*
  -> 未命中：创建 GCString
       -> 注册到关联 GarbageCollector
       -> 插入驻留表
       -> 返回新指针
```

`find()` 只查询而不创建；GC sweep 删除死亡字符串前，通过所属 `StringPool` 移除驻留表条目，防止悬垂指针。

## 4. 驻留入口

- Lexer 产生字符串字面量。
- `CONCAT` 和字符串标准库创建结果。
- C API 压入字符串。
- number-to-string 转换。

项目对短字符串和长字符串使用统一驻留策略。这与官方 Lua 5.1 只保证短字符串驻留的策略不同，详见 `docs/compatibility/lua51/behavior-differences.md`。
