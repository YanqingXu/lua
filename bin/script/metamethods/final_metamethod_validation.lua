-- Final metamethod validation test
print("=== Final Metamethod Validation Test ===")

-- Test 1: Basic table operations  
print("Test 1: Basic table operations")
local t = {}
t.x = 10
print("t.x =", t.x)

-- Test 2: Basic metatable operations
print("\nTest 2: Basic metatable operations")
local obj = {}
obj.name = "test"
local meta = {}
meta.type = "metatable"

print("obj.name =", obj.name)
print("meta.type =", meta.type)

setmetatable(obj, meta)
local retrieved = getmetatable(obj)
print("Metatable set and retrieved")

-- Test 3: __index metamethod with table
print("\nTest 3: __index metamethod with table")
local defaults = {}
defaults.default_value = 100
defaults.default_name = "DefaultName"
meta.__index = defaults

print("Set __index to defaults")
print("defaults.default_value =", defaults.default_value)
print("defaults.default_name =", defaults.default_name)

-- Test accessing non-existent keys
print("Testing __index:")
local result1 = obj.default_value
local result2 = obj.default_name
print("obj.default_value =", result1)
print("obj.default_name =", result2)

-- Test 4: __newindex metamethod
print("\nTest 4: __newindex metamethod")
local storage = {}
meta.__newindex = storage

print("Set __newindex to storage")
obj.new_field = "new_value"
print("Set obj.new_field = new_value")

if storage.new_field then
    print("storage.new_field =", storage.new_field)
else
    print("storage.new_field = nil")
end

-- Test 5: Existing field modification
print("\nTest 5: Existing field modification")
obj.name = "modified"
print("Set obj.name = modified")
print("obj.name =", obj.name)

if storage.name then
    print("storage.name =", storage.name)
else
    print("storage.name = nil (expected)")
end

print("\n=== Final validation completed ===")
print("Check if __index and __newindex are working correctly")
