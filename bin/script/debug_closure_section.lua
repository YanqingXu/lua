-- Test the closure section specifically
print("=== Closure Section Test ===")

-- Test statistics
local test_count = 0
local passed_count = 0
local failed_count = 0

-- Test helper function
local function test_assert(condition, test_name, expected, actual)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        failed_count = failed_count + 1
        local msg = "[Failed] " .. test_name .. " - Failed"
        if expected and actual then
            msg = msg .. " (Expected: " .. tostring(expected) .. ", Actual: " .. tostring(actual) .. ")"
        end
        print(msg)
        return false
    end
end

print("5. Closures and Upvalues Test")
print("------------------------------")

-- Simple closure
function create_counter(start)
    local count = start or 0
    return function()
        count = count + 1
        return count
    end
end

print("About to create counter...")
local counter = create_counter(10)
print("Counter created, calling first time...")
local count1 = counter()
print("First call completed, result:", count1)
print("Calling second time...")
local count2 = counter()
print("Second call completed, result:", count2)

test_assert(count1 == 11, "Closure counter (first call)", 11, count1)
test_assert(count2 == 12, "Closure counter (second call)", 12, count2)

print("About to test nested closure...")

-- Nested closure
function outer_func(outer_x)
    return function(outer_y)
        return function(outer_z)
            return outer_x + outer_y + outer_z
        end
    end
end

print("Calling nested closure...")
local nested_result = outer_func(1)(2)(3)
print("Nested closure result:", nested_result)
test_assert(nested_result == 6, "Nested closure", 6, nested_result)

print("=== Closure Section Test Results ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("=== Closure Section Test Complete ===")
