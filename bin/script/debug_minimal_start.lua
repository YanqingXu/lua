-- Minimal test to find the exact problem
print("=== Minimal Start Test ===")

-- Test 1: Basic print
print("Step 1: Basic print works")

-- Test 2: Variable assignment
local test_count = 0
print("Step 2: Variable assignment works")

-- Test 3: Function definition
local function test_assert(condition, test_name)
    print("test_assert called with: " .. test_name)
    return condition
end
print("Step 3: Function definition works")

-- Test 4: Function call
local result = test_assert(true, "test function call")
print("Step 4: Function call works, result:", result)

-- Test 5: String concatenation in function
local function test_with_concat(condition, test_name, expected, actual)
    local msg = "[Test] " .. test_name
    if expected and actual then
        msg = msg .. " (Expected: " .. tostring(expected) .. ", Actual: " .. tostring(actual) .. ")"
    end
    print(msg)
    return condition
end
print("Step 5: Function with string concatenation defined")

-- Test 6: Call function with concatenation
local concat_result = test_with_concat(true, "concat test", "expected", "actual")
print("Step 6: Function with concatenation called, result:", concat_result)

print("=== Minimal Start Test Complete ===")
