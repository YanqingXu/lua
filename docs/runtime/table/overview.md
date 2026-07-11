---
status: current
verified_against: docs/architecture/overview.md; src/core/table.hpp; src/core/table.cpp; src/core/metatable.hpp; src/core/metatable.cpp; tests/unit/core/test_table.cpp; tests/unit/metamethod/
last_checked: 2026-06-13
applies_to: Chinese table and metatable overview
---

# Table & Metatable Overview

## 1. 这个模块解决什么问题？

Table 是 Lua 唯一的数据结构，元表提供了运算符重载和面向对象支持。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/core/table.hpp/cpp` | Table 类（数组+哈希混合存储） |
| `src/core/metatable.hpp/cpp` | 元方法系统（17种元方法） |
| `src/vm/vm_table.cpp` | VM 表操作辅助函数 |
| `src/vm/vm_handlers/vm_handlers_table.cpp` | GETTABLE/SETTABLE 等指令 |

## 3. Table 设计

```
Table = 数组部分 + 哈希部分

数组部分: Vec<Value>
  - 连续正整数键 (1, 2, 3, ...)
  - O(1) 索引访问

哈希部分: HashMap<Value, Value>
  - 非整数键、字符串键、稀疏整数键
  - O(1) 平均查找
```

## 4. 元表系统

```
17 种元方法:
  __index, __newindex    — 表读写
  __add, __sub, __mul,
  __div, __mod, __pow,
  __unm                   — 算术
  __concat                — 拼接
  __eq, __lt, __le        — 比较
  __call                  — 当函数调用
  __len                   — # 运算符
  __gc                    — GC 终结器
  __mode                  — 弱表标记
  __tostring              — tostring()
  __metatable             — 元表保护
```
