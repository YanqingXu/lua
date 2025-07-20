-- Step 3: Test recursive function from comprehensive test
print("=== Step 3: Recursive Function Test ===")

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

-- Recursive function test
print("3. Recursive Function Test")
print("---------------------------")

-- Simple recursive function (factorial)
function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

print("Testing factorial(5)...")
local fact_result = factorial(5)
print("factorial(5) result:", fact_result)
test_assert(fact_result == 120, "Factorial function", 120, fact_result)

print("Step 3 completed!")
print("Tests: " .. test_count .. ", Passed: " .. passed_count .. ", Failed: " .. failed_count)
