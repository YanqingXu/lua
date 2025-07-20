-- Step 2: Test function functionality from comprehensive test
print("=== Step 2: Function Functionality ===")

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

-- Function Test
print("2. Function Test")
print("----------------")

-- Simple function
function add(a, b)
    return a + b
end

local sum_result = add(5, 3)
test_assert(sum_result == 8, "Simple function call", 8, sum_result)

-- Function with multiple parameters
function multiply_three(a, b, c)
    return a * b * c
end

local mult_result = multiply_three(2, 3, 4)
test_assert(mult_result == 24, "Function with multiple parameters", 24, mult_result)

-- Function with local variables
function calculate_area(width, height)
    local area = width * height
    return area
end

local area_result = calculate_area(5, 6)
test_assert(area_result == 30, "Function with local variables", 30, area_result)

print("Step 2 completed!")
print("Tests: " .. test_count .. ", Passed: " .. passed_count .. ", Failed: " .. failed_count)
