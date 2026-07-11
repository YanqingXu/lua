# Arithmetic Metamethods — 算术元方法

## 1. 算术运算符的元方法

| 运算符 | 元方法 | VM 指令 |
|--------|--------|---------|
| `a + b` | `__add` | ADD |
| `a - b` | `__sub` | SUB |
| `a * b` | `__mul` | MUL |
| `a / b` | `__div` | DIV |
| `a % b` | `__mod` | MOD |
| `a ^ b` | `__pow` | POW |
| `-a` | `__unm` | UNM |

## 2. 触发条件

```
算术元方法在以下情况下触发:

1. 两个操作数中至少有一个是 table/userdata
2. 该操作数的元表中有对应的元方法
3. (Lua 5.1 中算术元方法不检查两个操作数的元方法兼容性)

示例:
  local a = setmetatable({}, {
      __add = function(self, other)
          return { value = self.value + other.value }
      end
  })
  a + 3  -- 触发 __add
```

## 3. VM 侧实现

```cpp
// ADD 指令中:
Value& lhs = RK(B);
Value& rhs = RK(C);

// 先尝试数值运算
if (lhs.isNumber() && rhs.isNumber()) {
    R(A) = Value(lhs.asNumber() + rhs.asNumber());
    return;
}

// 尝试字符串转数字
// ...

// 尝试元方法
if (hasMetamethod(lhs, "__add")) {
    Value result = callTM(lhs, "__add", lhs, rhs);
    R(A) = result;
    return;
}
if (hasMetamethod(rhs, "__add")) {
    Value result = callTM(rhs, "__add", lhs, rhs);
    R(A) = result;
    return;
}

// 都不行 → 错误
throw RuntimeError("attempt to perform arithmetic on ...");
```

## 4. 示例: Vector 类

```lua
local Vector = {}
Vector.__index = Vector

function Vector:new(x, y)
    return setmetatable({ x = x, y = y }, self)
end

function Vector:__add(other)
    return Vector:new(self.x + other.x, self.y + other.y)
end

function Vector:__tostring()
    return string.format("Vector(%d, %d)", self.x, self.y)
end

local a = Vector:new(1, 2)
local b = Vector:new(3, 4)
local c = a + b  -- Vector(4, 6)
print(c)         -- Vector(4, 6)
```
