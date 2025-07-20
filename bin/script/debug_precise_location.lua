-- Precise location debugging for pcall issue
print("=== Precise Location Debug ===")

-- Test statistics
local test_count = 0
local passed_count = 0
local failed_count = 0

-- Test helper function (exact copy from comprehensive test)
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

print("8. Error Handling Test")
print("----------------------")

print("Step 1: pcall success case")
local success, pcall_result = pcall(function()
    return "successful_execution"
end)
test_assert(success == true, "pcall success case", true, success)
print("Step 1 completed")

print("Step 2: pcall return value test")
test_assert(pcall_result == "successful_execution", "pcall return value", "successful_execution", pcall_result)
print("Step 2 completed")

print("Step 3: About to start pcall error catching")
-- pcall error catching (exact copy from comprehensive test)
local error_success, error_msg = pcall(function()
    error("test_error")
end)
print("Step 3: pcall error catching completed")

print("Step 4: About to test error_success")
test_assert(error_success == false, "pcall error catching", false, error_success)
print("Step 4 completed")

print("Step 5: About to test error message type")
test_assert(type(error_msg) == "string", "pcall error message type", "string", type(error_msg))
print("Step 5 completed")

print("Step 6: All error handling tests completed")
print("")

print("=== Precise Location Debug Complete ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
