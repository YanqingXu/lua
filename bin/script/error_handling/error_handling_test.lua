print("=== Error Handling Test ===")

-- Test 1: Basic error handling
print("\nTest 1: Basic error handling")
local function test_error()
    error("This is a test error")
end

local success, err = pcall(test_error)
print("Success:", success)
print("Error:", err)

-- Test 2: Division by zero
print("\nTest 2: Division by zero")
local function divide_by_zero()
    local a = 10
    local b = 0
    return a / b
end

local success2, result2 = pcall(divide_by_zero)
print("Success:", success2)
print("Result:", result2)

-- Test 3: Function call error
print("\nTest 3: Function call error")
local function call_non_function()
    local not_a_function = 42
    return not_a_function()
end

local success3, err3 = pcall(call_non_function)
print("Success:", success3)
print("Error:", err3)

print("\n=== Error Handling Test Complete ===")
