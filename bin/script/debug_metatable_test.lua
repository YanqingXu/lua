-- Metatable test to check metatable functionality
print("=== Metatable Debug Test ===")

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

-- Test 1: Basic metatable operations
local obj = {}
local meta = {}
setmetatable(obj, meta)
local retrieved_meta = getmetatable(obj)
test_assert(retrieved_meta == meta, "Metatable set and get")

-- Test 2: __index metamethod
local defaults = {default_value = "default"}
meta.__index = defaults
test_assert(obj.default_value == "default", "__index metamethod")

-- Test 3: __newindex metamethod
local storage = {}
meta.__newindex = storage
obj.new_field = "new_value"
test_assert(storage.new_field == "new_value", "__newindex metamethod")

print("=== Metatable test completed ===")
