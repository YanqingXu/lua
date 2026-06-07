# Equality — 相等性比较

## 1. Value 相等

```cpp
// operator==
bool Value::operator==(const Value& other) const {
    // 类型不同 → false
    if (value_.index() != other.value_.index()) return false;
    // 相同类型 → variant 默认比较
    return value_ == other.value_;
}
```

## 2. 各类型的相等语义

| 类型 | 比较方式 | 示例 |
|------|---------|------|
| nil | `true` (唯一) | `nil == nil` |
| boolean | 值相等 | `true == true` |
| number | double 值相等 | `42.0 == 42.0` |
| string | **指针相等** (驻留) | `"abc" == "abc"` (同一 GCString*) |
| table | **指针相等** (引用) | `{} ~= {}` (不同对象) |
| function | **指针相等** (引用) | 同上 |
| userdata | **指针相等** (引用) | 同上 |
| thread | **指针相等** (引用) | 同上 |

## 3. NaN 特殊处理

```
NaN == NaN  → false  (IEEE 754 标准)
```

Lua 的 VM 比较指令 (EQ) 也遵循这一规则。

## 4. raw equality vs metamethod equality

```
== 运算符:
  1. 先比较类型
  2. 如果类型相同 → 值比较
  3. 如果类型不同 → 永远 false
  4. 不触发 __eq 元方法 (常规 == 检查)

但 OP_EQ 指令:
  - 先做常规比较
  - 如果类型不同且两边都有元表 → 尝试 __eq 元方法
```

## 5. 常见陷阱

```lua
-- 引用相等陷阱
local a = {}
local b = {}
a == b  -- false (不同对象)

local function f() end
local function g() end
f == g  -- false (不同闭包)

-- 字符串相等是安全的 (驻留)
"abc" == "abc"  -- true (同一 GCString*)
```
