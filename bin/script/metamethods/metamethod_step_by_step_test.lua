-- Metamethod step by step test
print("=== Metamethod Step by Step Test ===")

-- Test 1: Create objects step by step
print("Test 1: Create objects step by step")
local obj = {}
local meta = {}
local defaults = {}

print("Created empty tables")

-- Test 2: Set up default values
print("Test 2: Set up default values")
defaults.x = 100
defaults.y = 200
print("defaults.x =", defaults.x)
print("defaults.y =", defaults.y)

-- Test 3: Set up metatable
print("Test 3: Set up metatable")  
meta.__index = defaults
setmetatable(obj, meta)
print("Set __index and metatable")

-- Test 4: Test __index access
print("Test 4: Test __index access")
print("Accessing obj.x (should be 100):")
local result_x = obj.x
print("obj.x =", result_x)

print("Accessing obj.y (should be 200):")
local result_y = obj.y
print("obj.y =", result_y)

-- Test 5: Test __newindex
print("Test 5: Test __newindex")
local storage = {}
meta.__newindex = storage
print("Set __newindex to storage")

print("Setting obj.new_key = 'new_value':")
obj.new_key = "new_value"
print("storage.new_key =", storage.new_key)

-- Test 6: Test existing field (should not go to __newindex)
print("Test 6: Test existing field")
obj.existing = "exists"
print("obj.existing =", obj.existing)

obj.existing = "modified"
print("Modified obj.existing =", obj.existing)
print("storage.existing =", storage.existing)

print("=== Metamethod step by step test completed ===")
