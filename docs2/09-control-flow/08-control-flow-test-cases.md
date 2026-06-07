# Control Flow Test Cases

## 1. 基本控制流测试

```lua
-- If/Else
if true then assert(true)
else assert(false) end

if false then assert(false)
else assert(true) end

-- While
local i = 0
while i < 10 do i = i + 1 end
assert(i == 10)

-- Repeat
local i = 0
repeat i = i + 1 until i == 10
assert(i == 10)

-- For
local sum = 0
for i = 1, 10 do sum = sum + i end
assert(sum == 55)

-- Break
local x = 0
while true do
    x = x + 1
    if x == 5 then break end
end
assert(x == 5)
```

## 2. 相关测试文件

- `tests/unit/compiler/test_codegen_conditions.cpp`
- `tests/unit/compiler/test_codegen_characterization.cpp`
- `tests/lua/official/constructs.lua`
- `examples/control_flow.lua`
