-- Test upvalue fix
print("=== Upvalue Fix Test ===")

print("Test 1: Simple upvalue")
local x = 42
print("Before function: x =", x)

function test()
    print("Inside function: x =", x)
    return x
end

local result = test()
print("Function returned:", result)
print("Expected: 42, Got:", result)

print("\nTest 2: String upvalue")
local message = "hello world"
print("Before function: message =", message)

function test_string()
    print("Inside function: message =", message)
    return message
end

local result2 = test_string()
print("Function returned:", result2)
print("Expected: hello world, Got:", result2)

print("\n=== Fix test completed ===")
