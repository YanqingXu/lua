-- Step 4: Test closure functionality from comprehensive test
print("=== Step 4: Closure Test ===")

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

-- Closure test
print("4. Closure Test")
print("---------------")

-- Simple closure
print("Creating counter...")
function create_counter(start)
    local count = start or 0
    return function()
        count = count + 1
        return count
    end
end

print("Testing closure creation...")
local counter = create_counter(10)
print("Counter created, calling first time...")
local count1 = counter()
print("First call result:", count1)

print("Step 4 completed!")
print("Tests: " .. test_count .. ", Passed: " .. passed_count .. ", Failed: " .. failed_count)
