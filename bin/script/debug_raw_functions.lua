-- Debug raw functions test
print("=== Debug Raw Functions Test ===")

-- Test helper function
local function test_assert(condition, test_name)
    if condition then
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        print("[Failed] " .. test_name .. " - Failed")
        return false
    end
end

print("Testing rawget...")
local t = {existing = "value"}
local result = rawget(t, "existing")
test_assert(result == "value", "rawget existing key")

local nil_result = rawget(t, "nonexistent")
test_assert(nil_result == nil, "rawget nonexistent key")

print("Testing rawset...")
rawset(t, "new_key", "new_value")
local new_result = rawget(t, "new_key")
test_assert(new_result == "new_value", "rawset new key")

print("Testing rawlen...")
local array = {1, 2, 3, 4, 5}
local len_result = rawlen(array)
test_assert(len_result == 5, "rawlen array")

local str_len = rawlen("hello")
test_assert(str_len == 5, "rawlen string")

print("Testing rawequal...")
local obj1 = {}
local obj2 = {}
test_assert(rawequal(obj1, obj1) == true, "rawequal same object")
test_assert(rawequal(obj1, obj2) == false, "rawequal different objects")
test_assert(rawequal(5, 5) == true, "rawequal same numbers")

print("=== Raw functions test completed ===")
