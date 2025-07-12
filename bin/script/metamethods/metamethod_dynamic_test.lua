-- Metamethod test with dynamic assignment
print("=== Metamethod Test with Dynamic Assignment ===")

-- Test __index with dynamic assignment
print("Test 1: __index with dynamic assignment")
local obj = {}
local meta = {}
local defaults = {}

-- Use dynamic assignment instead of table literals
obj.name = "test"
meta.type = "metatable"
defaults.default_value = 100
defaults.default_name = "DefaultName"

print("obj.name =", obj.name)
print("meta.type =", meta.type)
print("defaults.default_value =", defaults.default_value)
print("defaults.default_name =", defaults.default_name)

-- Set up __index
meta.__index = defaults
setmetatable(obj, meta)

print("Set __index and metatable")

-- Test __index
print("Testing __index:")
local result1 = obj.default_value
local result2 = obj.default_name
print("obj.default_value =", result1)
print("obj.default_name =", result2)

-- Test __newindex
print("Test 2: __newindex with dynamic assignment")
local storage = {}
meta.__newindex = storage

print("Set __newindex")
obj.new_field = "new_value"
print("Set obj.new_field")

print("storage.new_field =", storage.new_field)

-- Test existing field modification
print("Test 3: Existing field modification")
obj.name = "modified"
print("obj.name =", obj.name)
print("storage.name =", storage.name)

print("=== Metamethod test completed ===")
