-- Function test to check function functionality
print("=== Function Debug Test ===")

-- Test 1: Simple function definition
function add(a, b)
    return a + b
end

print("Test 1: Function defined")

-- Test 2: Function call
local result = add(5, 3)
print("Test 2: add(5, 3) =", result)

-- Test 3: Test helper function
local function test_assert(condition, test_name)
    if condition then
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        print("[Failed] " .. test_name .. " - Failed")
        return false
    end
end

print("Test 3: Test helper function defined")

-- Test 4: Use test helper
test_assert(result == 8, "Addition function test")

print("=== Function test completed ===")
