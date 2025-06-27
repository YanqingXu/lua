-- Simple upvalue debug test
print("=== Upvalue Debug Test ===")

-- Test 1: Very simple upvalue
print("Test 1: Simple upvalue")
local x = "hello"
print("Before function, x =", x)

function test()
    print("Inside function, x =", x)
    return x
end

local result = test()
print("Function returned:", result)

-- Test 2: Global vs upvalue
print("\nTest 2: Global vs upvalue")
global_var = "global_value"
local local_var = "local_value"

function test_global()
    print("Global var:", global_var)
    return global_var
end

function test_upvalue()
    print("Local var (upvalue):", local_var)
    return local_var
end

print("Global test:", test_global())
print("Upvalue test:", test_upvalue())

print("=== Debug test completed ===")
