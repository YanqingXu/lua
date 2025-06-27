-- VM debug test for upvalue binding
print("=== VM Debug Test ===")

local x = 42
print("x defined with value:", x)

function test()
    return x
end

print("Function defined, calling...")
local result = test()
print("Result:", result)

print("=== VM debug test completed ===")
