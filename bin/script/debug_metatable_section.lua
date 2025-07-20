-- Test the metatable section specifically
print("=== Metatable Section Test ===")

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

print("6. Metatable and Metamethods Test")
print("----------------------------------")

-- Basic metatable operations
print("Testing basic metatable operations...")
local meta_obj = {}
local meta_table = {}
setmetatable(meta_obj, meta_table)
local retrieved_meta = getmetatable(meta_obj)
test_assert(retrieved_meta == meta_table, "Metatable set and get", "same", "same")

-- __index metamethod
print("Testing __index metamethod...")
local defaults = {default_value = "default_value"}
meta_table.__index = defaults
test_assert(meta_obj.default_value == "default_value", "__index metamethod", "default_value", meta_obj.default_value)

-- __newindex metamethod
print("Testing __newindex metamethod...")
local storage = {}
meta_table.__newindex = storage
meta_obj.new_field = "new_value"
test_assert(storage.new_field == "new_value", "__newindex metamethod", "new_value", storage.new_field)

-- __tostring metamethod
print("Testing __tostring metamethod...")
local tostring_obj = {}
local tostring_meta = {
    __tostring = function(tostring_obj_param)
        return "custom_string_representation"
    end
}
setmetatable(tostring_obj, tostring_meta)
local tostring_result = tostring(tostring_obj)
test_assert(tostring_result == "custom_string_representation", "__tostring metamethod", "custom_string_representation", tostring_result)

print("=== Metatable Section Test Results ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("=== Metatable Section Test Complete ===")
