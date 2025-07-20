-- Comprehensive Feature Validation Test File - SECOND HALF
-- Tests advanced features
-- Version: 1.0 (July 20, 2025)

print("=== Lua Interpreter Comprehensive Feature Validation Test - SECOND HALF ===")
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
-- 6. Metatable and Metamethods Test
-- ============================================
print("6. Metatable and Metamethods Test")
print("----------------------------------")

-- Basic metatable operations
local meta_obj = {}
local meta_table = {}
setmetatable(meta_obj, meta_table)
local retrieved_meta = getmetatable(meta_obj)
test_assert(retrieved_meta == meta_table, "Metatable set and get", "same", "same")

-- __index metamethod
local defaults = {default_value = "default_value"}
meta_table.__index = defaults
test_assert(meta_obj.default_value == "default_value", "__index metamethod", "default_value", meta_obj.default_value)

-- __newindex metamethod
local storage = {}
meta_table.__newindex = storage
meta_obj.new_field = "new_value"
test_assert(storage.new_field == "new_value", "__newindex metamethod", "new_value", storage.new_field)

-- __tostring metamethod
local tostring_obj = {}
local tostring_meta = {
    __tostring = function(tostring_obj_param)
        return "custom_string_representation"
    end
}
setmetatable(tostring_obj, tostring_meta)
local tostring_result = tostring(tostring_obj)
test_assert(tostring_result == "custom_string_representation", "__tostring metamethod", "custom_string_representation", tostring_result)

print("")

-- ============================================
-- 7. Standard Library Functions Test
-- ============================================
print("7. Standard Library Functions Test")
print("-----------------------------------")

-- Base library functions
test_assert(tostring(42) == "42", "tostring function", "42", tostring(42))
test_assert(tonumber("123") == 123, "tonumber function", 123, tonumber("123"))

-- Math library functions
test_assert(math.abs(-5) == 5, "math.abs function", 5, math.abs(-5))
test_assert(math.max(1, 5, 3) == 5, "math.max function", 5, math.max(1, 5, 3))
test_assert(math.min(1, 5, 3) == 1, "math.min function", 1, math.min(1, 5, 3))
test_assert(math.floor(3.7) == 3, "math.floor function", 3, math.floor(3.7))
test_assert(math.ceil(3.2) == 4, "math.ceil function", 4, math.ceil(3.2))

-- String library functions
test_assert(string.len("hello") == 5, "string.len function", 5, string.len("hello"))
test_assert(string.upper("hello") == "HELLO", "string.upper function", "HELLO", string.upper("hello"))
test_assert(string.lower("WORLD") == "world", "string.lower function", "world", string.lower("WORLD"))
test_assert(string.sub("hello", 2, 4) == "ell", "string.sub function", "ell", string.sub("hello", 2, 4))

print("")

-- ============================================
-- SECOND HALF RESULTS
-- ============================================
print("=== SECOND HALF Test Results ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("Success Rate: " .. (passed_count / test_count * 100) .. "%")
print("SECOND HALF completed successfully!")
print("=== End SECOND HALF Test ===")
