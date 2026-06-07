# Table Test Cases — 表测试用例

## 1. 基本读写

```lua
local t = {}
t[1] = "a"
t["1"] = "b"      -- "1" ≠ 1
t[3.14] = "pi"
t[true] = "bool"

assert(t[1] == "a")
assert(t["1"] == "b")
assert(t[3.14] == "pi")
assert(t[true] == "bool")
```

## 2. 数组和长度

```lua
local t = {10, 20, 30}
assert(#t == 3)

-- 带洞的数组
local t = {10, nil, 30}
-- #t 可以是 1 或 3 (未定义)

-- 空表
local t = {}
assert(#t == 0)

-- 稀疏表
local t = {}
t[5] = "five"
t[10] = "ten"
-- #t 可能是 0, 5, 10 之一 (取决于边界)
```

## 3. 元表

```lua
-- __index table
local defaults = { health = 100, mana = 50 }
local player = setmetatable({}, { __index = defaults })
assert(player.health == 100)

-- __newindex
local t = setmetatable({}, {
    __newindex = function(_, k, v)
        error("read-only table")
    end
})
-- t.x = 1  → error

-- __add
local v1 = setmetatable({x=1,y=2}, { __add = function(a,b) return a.x+b.x end })
local v2 = setmetatable({x=3,y=4}, { __add = function(a,b) return a.x+b.x end })
-- v1 + v2 → 4
```

## 4. pairs / ipairs

```lua
local t = { a=1, b=2, c=3 }
local count = 0
for k, v in pairs(t) do count = count + 1 end
assert(count == 3)

local t = {10, 20, 30, 40, 50}
local sum = 0
for i, v in ipairs(t) do sum = sum + v end
assert(sum == 150)
```

## 5. 相关测试文件

- `tests/unit/core/test_table.cpp`
- `tests/unit/metamethod/test_metamethod_arith.cpp`
- `tests/unit/metamethod/test_metamethod_complete.cpp`
- `tests/lua/official/literals.lua`
- `tests/lua/official/nextvar.lua`
