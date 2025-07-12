-- Comprehensive metamethod test after compilation fixes
print("=== Comprehensive Metamethod Test ===")

-- Test 1: Basic metatable operations
print("Test 1: Basic metatable operations")
local obj = {existing = "original"}
local meta = {}

print("Before setmetatable:")
print("  obj.existing = " .. obj.existing)
local mt_before = getmetatable(obj)
print("  getmetatable(obj) = " .. tostring(mt_before))

setmetatable(obj, meta)
print("After setmetatable:")
print("  getmetatable(obj) == meta: " .. tostring(getmetatable(obj) == meta))

-- Test 2: __index metamethod with table
print("")
print("Test 2: __index metamethod with table")
local defaults = {
    default_x = 100,
    default_y = 200,
    default_name = "DefaultObject"
}
meta.__index = defaults

print("  obj.existing = " .. obj.existing)  -- Should find in obj
print("  obj.default_x = " .. tostring(obj.default_x))  -- Should find in defaults
print("  obj.default_name = " .. tostring(obj.default_name))  -- Should find in defaults
print("  obj.nonexistent = " .. tostring(obj.nonexistent))  -- Should be nil

-- Test 3: __newindex metamethod with table
print("")
print("Test 3: __newindex metamethod with table")
local storage = {}
meta.__newindex = storage

print("  Setting obj.new_field = 'stored_value'")
obj.new_field = "stored_value"
print("  obj.new_field = " .. tostring(obj.new_field))
print("  storage.new_field = " .. tostring(storage.new_field))

print("  Setting obj.another_field = 42")
obj.another_field = 42
print("  obj.another_field = " .. tostring(obj.another_field))
print("  storage.another_field = " .. tostring(storage.another_field))

-- Test 4: Modifying existing field (should not trigger __newindex)
print("")
print("Test 4: Modifying existing field")
print("  Before: obj.existing = " .. obj.existing)
obj.existing = "modified"
print("  After: obj.existing = " .. obj.existing)
print("  storage.existing = " .. tostring(storage.existing))  -- Should be nil

-- Test 5: Remove metatable
print("")
print("Test 5: Remove metatable")
setmetatable(obj, nil)
print("  After setmetatable(obj, nil):")
print("  getmetatable(obj) = " .. tostring(getmetatable(obj)))
print("  obj.default_x = " .. tostring(obj.default_x))  -- Should be nil now

-- Test 6: Multiple objects with same metatable
print("")
print("Test 6: Multiple objects with same metatable")
local obj1 = {id = 1}
local obj2 = {id = 2}
local shared_meta = {shared_value = "shared"}
shared_meta.__index = shared_meta

setmetatable(obj1, shared_meta)
setmetatable(obj2, shared_meta)

print("  obj1.id = " .. obj1.id)
print("  obj1.shared_value = " .. tostring(obj1.shared_value))
print("  obj2.id = " .. obj2.id)
print("  obj2.shared_value = " .. tostring(obj2.shared_value))
print("  getmetatable(obj1) == getmetatable(obj2): " .. tostring(getmetatable(obj1) == getmetatable(obj2)))

print("")
print("=== Comprehensive test completed ===")
print("All metamethod operations completed successfully!")
