-- Debug pcall error catching exit issue
print("=== pcall Exit Issue Debug ===")

-- Test helper function
local function test_assert(condition, test_name, expected, actual)
    if condition then
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        local msg = "[Failed] " .. test_name .. " - Failed"
        if expected and actual then
            msg = msg .. " (Expected: " .. tostring(expected) .. ", Actual: " .. tostring(actual) .. ")"
        end
        print(msg)
        return false
    end
end

print("Step 1: pcall success case")
local success, pcall_result = pcall(function()
    return "successful_execution"
end)
test_assert(success == true, "pcall success case", true, success)
test_assert(pcall_result == "successful_execution", "pcall return value", "successful_execution", pcall_result)

print("Step 2: About to test pcall error catching...")
print("This is the problematic part that causes exit")

-- pcall error catching (the problematic code)
local error_success, error_msg = pcall(function()
    error("test_error")
end)

print("Step 3: pcall error catching completed")
print("error_success:", error_success)
print("error_msg:", error_msg)

test_assert(error_success == false, "pcall error catching", false, error_success)
test_assert(type(error_msg) == "string", "pcall error message type", "string", type(error_msg))

print("Step 4: All pcall tests completed successfully")
print("=== pcall Exit Issue Debug Complete ===")
