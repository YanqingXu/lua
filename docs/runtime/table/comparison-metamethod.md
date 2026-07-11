# Comparison Metamethods — 比较元方法

## 1. 比较元方法

| 运算符 | 元方法 | VM 指令 |
|--------|--------|---------|
| `a == b` | `__eq` | EQ |
| `a < b` | `__lt` | LT |
| `a <= b` | `__le` | LE |

## 2. __eq

```
a == b 的语义:
  1. 如果类型相同 → 值比较
  2. 如果类型不同且都有 __eq → 调用 __eq(a, b)
  3. 否则 → false

注意: 
  __eq 只在两个操作数的元表中有相同的 __eq 函数时才调用
  (这是 Lua 5.1 的行为)
```

## 3. __lt / __le

```
a < b:
  1. 如果都是 number → 数值比较
  2. 如果都是 string → 字典序比较
  3. 否则尝试 __lt 元方法

a <= b:
  1. 如果都是 number → 数值比较
  2. 如果都是 string → 字典序比较
  3. 否则尝试 __le 元方法
  4. 如果没有 __le → 尝试 __lt: a <= b ≡ not (b < a)
```

## 4. 示例

```lua
local mt = {
    __eq = function(a, b) return a.value == b.value end,
    __lt = function(a, b) return a.value < b.value end,
    __le = function(a, b) return a.value <= b.value end,
}

local a = setmetatable({ value = 10 }, mt)
local b = setmetatable({ value = 10 }, mt)
local c = setmetatable({ value = 20 }, mt)

print(a == b)  -- true (值相等)
print(a < c)   -- true (10 < 20)
print(c <= a)  -- false
```
