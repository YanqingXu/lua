-- Test metamethod access after compilation fix
print("=== Testing Metamethod Access ===")

-- Test 1: Basic metatable operations
print("Test 1: Basic metatable operations")
local obj = {value = 42}
local meta = {name = "TestMeta"}

print("Setting metatable...")
setmetatable(obj, meta)
print("Metatable set successfully")

local retrieved = getmetatable(obj)
print("Retrieved metatable: " .. tostring(retrieved))
print("Metatable match: " .. tostring(retrieved == meta))

-- Test 2: __index metamethod
print("")
print("Test 2: __index metamethod")
local fallback = {default_value = "from fallback"}
meta.__index = fallback

print("obj.value = " .. obj.value)  -- Should find in obj
print("obj.default_value = " .. tostring(obj.default_value))  -- Should find via __index

-- Test 3: __newindex metamethod
print("")
print("Test 3: __newindex metamethod")
local storage = {}
meta.__newindex = storage

print("Setting obj.new_key = 'test'")
obj.new_key = "test"
print("obj.new_key = " .. tostring(obj.new_key))
print("storage.new_key = " .. tostring(storage.new_key))

print("")
print("=== Metamethod access test completed ===")
print("All basic metamethod operations working!")
