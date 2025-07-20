-- Comprehensive Feature Validation Test File - ADVANCED FEATURES
-- Tests the most complex features that might cause issues
-- Version: 1.0 (July 20, 2025)

print("=== Advanced Features Test ===")
print("")

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

-- ============================================
-- 8. Error Handling Test
-- ============================================
print("8. Error Handling Test")
print("----------------------")

-- pcall test
local success, pcall_result = pcall(function()
    return "successful_execution"
end)
test_assert(success == true, "pcall success case", true, success)
test_assert(pcall_result == "successful_execution", "pcall return value", "successful_execution", pcall_result)

-- pcall error catching
local error_success, error_msg = pcall(function()
    error("test_error")
end)
test_assert(error_success == false, "pcall error catching", false, error_success)
test_assert(type(error_msg) == "string", "pcall error message type", "string", type(error_msg))

print("")

-- ============================================
-- 9. Advanced Metamethods Test
-- ============================================
print("9. Advanced Metamethods Test")
print("-----------------------------")

-- __call metamethod
local callable_obj = {}
local callable_meta = {
    __call = function(self, call_x, call_y)
        return call_x + call_y + 100
    end
}
setmetatable(callable_obj, callable_meta)
local call_result = callable_obj(5, 3)
test_assert(call_result == 108, "__call metamethod", 108, call_result)

-- __eq metamethod
local eq_obj1 = {value = 10}
local eq_obj2 = {value = 10}
local eq_meta = {
    __eq = function(eq_a, eq_b)
        return eq_a.value == eq_b.value
    end
}
setmetatable(eq_obj1, eq_meta)
setmetatable(eq_obj2, eq_meta)
test_assert(eq_obj1 == eq_obj2, "__eq metamethod", true, eq_obj1 == eq_obj2)

-- __concat metamethod
local concat_obj1 = {text = "Hello"}
local concat_obj2 = {text = "World"}
local concat_meta = {
    __concat = function(concat_a, concat_b)
        return concat_a.text .. " " .. concat_b.text
    end
}
setmetatable(concat_obj1, concat_meta)
setmetatable(concat_obj2, concat_meta)
local concat_result = concat_obj1 .. concat_obj2
test_assert(concat_result == "Hello World", "__concat metamethod", "Hello World", concat_result)

print("")

-- ============================================
-- ADVANCED FEATURES RESULTS
-- ============================================
print("=== Advanced Features Test Results ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("Success Rate: " .. (passed_count / test_count * 100) .. "%")
print("Advanced features test completed!")
print("=== End Advanced Features Test ===")
