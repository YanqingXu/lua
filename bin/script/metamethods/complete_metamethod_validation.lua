-- Complete metamethod functionality validation
print("=== Complete Metamethod Validation ===")

-- Test 1: Basic metatable operations
print("Test 1: Basic metatable operations")
local obj = {}
obj.name = "TestObject"
print("Created obj with obj.name = " .. obj.name)

local meta = {}
print("Created metatable: " .. tostring(meta))

-- Test 2: setmetatable/getmetatable
print("")
print("Test 2: setmetatable/getmetatable")
local before_mt = getmetatable(obj)
print("Before setmetatable: " .. tostring(before_mt))

setmetatable(obj, meta)
local after_mt = getmetatable(obj)
print("After setmetatable: " .. tostring(after_mt))
print("Metatable set correctly: " .. tostring(after_mt == meta))

-- Test 3: __index metamethod with table
print("")
print("Test 3: __index metamethod with table")
local defaults = {}
defaults.default_value = 100
defaults.default_name = "Default"
meta.__index = defaults

print("obj.name = " .. tostring(obj.name))  -- Should find in obj
print("obj.default_value = " .. tostring(obj.default_value))  -- Should find via __index
print("obj.default_name = " .. tostring(obj.default_name))  -- Should find via __index

-- Test 4: __newindex metamethod
print("")
print("Test 4: __newindex metamethod")
local storage = {}
meta.__newindex = storage

print("Setting obj.new_field = 'stored'")
obj.new_field = "stored"
print("obj.new_field = " .. tostring(obj.new_field))
print("storage.new_field = " .. tostring(storage.new_field))

-- Test 5: Existing field modification (should not use __newindex)
print("")
print("Test 5: Existing field modification")
print("Before: obj.name = " .. tostring(obj.name))
obj.name = "ModifiedName"
print("After: obj.name = " .. tostring(obj.name))
print("storage.name = " .. tostring(storage.name))  -- Should be nil

-- Test 6: Multiple objects with shared metatable
print("")
print("Test 6: Multiple objects with shared metatable")
local obj1 = {}
local obj2 = {}
obj1.id = 1
obj2.id = 2

local shared_meta = {}
shared_meta.shared_property = "shared_value"
shared_meta.__index = shared_meta

setmetatable(obj1, shared_meta)
setmetatable(obj2, shared_meta)

print("obj1.id = " .. tostring(obj1.id))
print("obj1.shared_property = " .. tostring(obj1.shared_property))
print("obj2.id = " .. tostring(obj2.id))
print("obj2.shared_property = " .. tostring(obj2.shared_property))

local mt1 = getmetatable(obj1)
local mt2 = getmetatable(obj2)
print("Same metatable: " .. tostring(mt1 == mt2))

-- Test 7: Remove metatable
print("")
print("Test 7: Remove metatable")
setmetatable(obj, nil)
local removed_mt = getmetatable(obj)
print("After removing metatable: " .. tostring(removed_mt))
print("obj.default_value = " .. tostring(obj.default_value))  -- Should be nil now

print("")
print("=== All metamethod validation tests completed successfully! ===")
