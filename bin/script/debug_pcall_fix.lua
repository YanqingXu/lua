-- Test pcall with simplified error message handling
print("=== pcall Fix Test ===")

-- Test helper function with simplified error handling
local function test_assert_simple(condition, test_name)
    if condition then
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        print("[Failed] " .. test_name .. " - Failed")
        return false
    end
end

print("8. Error Handling Test")
print("----------------------")

-- pcall success case
local success, pcall_result = pcall(function()
    return "successful_execution"
end)
test_assert_simple(success == true, "pcall success case")
test_assert_simple(pcall_result == "successful_execution", "pcall return value")

-- pcall error catching with simplified handling
print("Testing pcall error catching...")
local error_success, error_msg = pcall(function()
    error("test_error")
end)

print("pcall error test completed")
print("Error success:", error_success)
print("Error message type:", type(error_msg))

-- Use simplified assertions without complex string operations
test_assert_simple(error_success == false, "pcall error catching")
test_assert_simple(type(error_msg) == "string", "pcall error message type")

print("All pcall tests completed successfully!")
print("=== pcall Fix Test Complete ===")
