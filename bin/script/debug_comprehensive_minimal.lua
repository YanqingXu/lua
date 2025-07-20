-- Minimal version of comprehensive test to find the exact problem
print("=== Minimal Comprehensive Test ===")

-- Copy the exact first few lines from comprehensive_validation_test.lua
print("=== Lua Interpreter Comprehensive Feature Validation Test ===")
print("Project Completion: 98% (All core features except coroutines)")
print("")

-- Test statistics
local test_count = 0
local passed_count = 0
local failed_count = 0

print("Variables created successfully")

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

print("Test helper function created successfully")

-- Test the helper function
test_assert(true, "Test helper function", true, true)

print("=== Minimal Comprehensive Test Complete ===")
