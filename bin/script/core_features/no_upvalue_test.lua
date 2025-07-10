-- Test without upvalues
print("=== No Upvalue Test ===")

-- Global variables only
global_var = "hello"

function test_global()
    print("Global var:", global_var)
    return global_var
end

print("Testing global variable access...")
local result = test_global()
print("Result:", result)

-- Simple function with parameters
function add(a, b)
    return a + b
end

print("Testing function with parameters...")
local sum = add(5, 3)
print("5 + 3 =", sum)

print("=== Test completed ===")
