-- Multi-return test to check multi-return functionality
print("=== Multi-return Debug Test ===")

-- Test helper function
local function test_assert(condition, test_name)
    if condition then
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        print("[Failed] " .. test_name .. " - Failed")
        return false
    end
end

-- Test 1: Multi-return function
function multi_return()
    return 1, 2, 3
end

print("Multi-return function defined")

-- Test 2: Multi-return assignment
local a, b, c = multi_return()
print("Values assigned: a =", a, "b =", b, "c =", c)

-- Test 3: Check values
test_assert(a == 1, "Multi-return value a")
test_assert(b == 2, "Multi-return value b")
test_assert(c == 3, "Multi-return value c")

-- Test 4: Combined condition
test_assert(a == 1 and b == 2 and c == 3, "Multi-return combined check")

print("=== Multi-return test completed ===")
