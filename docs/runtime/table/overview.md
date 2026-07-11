---
status: current
verified_against: src/core/table.hpp; src/core/table.cpp; src/core/metatable.hpp; src/core/metatable.cpp; src/vm/vm_ops.cpp; src/vm/vm_table.cpp; src/vm/vm_handlers/vm_handlers_table.cpp; src/vm/vm_handlers/vm_handlers_arith.cpp; tests/unit/core/test_table.cpp; tests/unit/metamethod/; tests/lua/tables/; src/core/; src/runtime/; src/vm/; tests/unit/core/; tests/unit/vm/; tests/lua/runtime/
last_checked: 2026-07-11
applies_to: Table 存储、遍历与 Metamethod 分派
---

# Table 存储、遍历与 Metamethod 分派

Table 同时承担数组、字典、对象和模块命名空间。实现需要把原始存储操作与可能执行 Lua 代码的 metamethod 分派分开，否则查找、哈希和递归保护会互相污染。

## 1. 存储模型

Table 由适合正整数键的 array part 和通用 key 的 hash part 组成：

```text
Table
├── array[1..n]       # 密集正整数键
├── hash              # 其他合法 Value key
└── metatable         # 可选 Table*
```

具体扩容策略由 `src/core/table.*` 负责。对外语义不保证遍历顺序，也不允许调用者依赖某个整数键当前落在 array 还是 hash。

### 键规范化

- nil 不能作为写入键；
- NaN 不能作为稳定键；
- 可精确表示的正整数 number 可以进入 array part；
- -0/+0 的相等与哈希必须规范一致；
- string 驻留允许内部身份快速路径；
- table/function/userdata 等对象键按身份。

哈希函数与原始相等必须闭合：若两个 key 相等，它们必须产生相同 hash。

## 2. 原始 get/set

`Table::get/set` 只操作存储，不触发 Lua metamethod。VM 层的 `getTable/setTable` 实现语言语义：

```text
GET:
  raw value exists? → return it
  no __index?       → nil
  __index function? → call(table, key)
  __index table?    → repeat lookup on that table

SET:
  raw key exists?     → update directly
  no __newindex?      → raw insert
  __newindex function → call(table, key, value)
  __newindex table    → repeat assignment on that table
```

链式查找需要循环/深度保护。保护计数属于一次语言操作，不应永久存进 Table 对象。

## 3. Metatable 与缓存

metatable 也是普通 Table，但其约定字段由 `TMS` 枚举和查找辅助统一管理。可缓存“某 metamethod 不存在”以减少热点查询；写入 metatable 后必须使相关缺失缓存失效。

基础类型的共享 metatable 由全局运行时状态持有；table/userdata 可以有对象级 metatable。调试 API 修改 metatable 时仍必须维护同样的缓存不变量。

## 4. 运算与比较 metamethod

| 操作 | 直接路径 | fallback |
|---|---|---|
| `+ - * / % ^` | 两个 number | `__add` ... `__pow` |
| unary `-` | number | `__unm` |
| concat | string/number 可拼接 | `__concat` |
| length | string/table 快路径 | `__len`（按项目兼容策略） |
| `< <=` | number 或 string | `__lt` / `__le`，必要时兼容 fallback |
| `==` | 原始相等 | 符合 Lua 约束时 `__eq` |
| call | Function | `__call`，原对象成为首参数 |

二元 metamethod 的选择顺序必须集中实现，避免每个 opcode 对左右操作数采用不同规则。metamethod 返回值也要经过正常 call pipeline，因此可能 yield、抛错或触发 GC。

`Value::operator==` 与 Table 哈希查找不能调用 `__eq`；带副作用的语言比较由 VM 显式执行。

## 5. `next`、`pairs` 与 `ipairs`

`next(table, key)` 在 table 当前存储中寻找 key 的后继；key 必须是 nil（开始）或当前存在的合法键。array 与 hash 的内部过渡不构成对外稳定顺序。

Lua 5.1 中 `pairs` 返回 `next, table, nil`。`ipairs` 从 1 开始按连续整数读取，在第一个 nil 处停止。修改 table 结构期间继续遍历的行为只保证 Lua 规范允许的范围，文档和测试不应锁定 unordered container 的桶顺序。

## 6. 构造与 `SETLIST`

table constructor 中记录字段和列表字段具有不同生成路径。连续列表值可由 `SETLIST` 批量写入 array 区；末项函数调用/vararg 可能产生开放数量值。大块索引使用扩展 operand 时，Compiler 与 VM 必须对额外指令消费保持一致。

## 7. GC、弱表和终结

普通 table 的 key、value 与 metatable 都是强边。弱模式由 metatable 的 `__mode` 决定，GC 在专门阶段处理弱 key/value，不能通过普通 get/set 临时跳过追踪来模拟。详见 [GC 实现](../../gc/implementation.md)。

## 8. 性能与正确性边界

- 先用 raw path 确认存储，再进入 metamethod；不能为命中键调用 `__index`。
- 扩容后所有 Value 与对象边仍可被 GC 扫描。
- 查找链有明确上限或循环检测。
- 不向文档承诺遍历顺序、桶数量或 array/hash 分界。
- 热路径避免复制 Table，不用 `shared_ptr` 改写 GC 身份模型。
- metamethod 调用前后重新获取可能因栈扩容失效的引用。

这些不变量由 `test_table.cpp`、`tests/unit/metamethod/` 和 `tests/lua/tables/` 共同验证。
