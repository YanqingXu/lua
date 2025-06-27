-- Minimal upvalue test
print("=== Minimal Upvalue Test ===")

local x = 42

function test()
    return x
end

print("Testing upvalue...")
local result = test()
print("Result:", result)
