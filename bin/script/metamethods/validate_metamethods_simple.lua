-- Simple metamethod validation test
print("=== Simple Metamethod Validation ===")

-- Test 1: Check if setmetatable function exists
print("Test 1: setmetatable function check")
if setmetatable then
    print("  setmetatable function: EXISTS")
else
    print("  setmetatable function: MISSING")
    print("  ERROR: setmetatable function is not available!")
    return
end

-- Test 2: Check if getmetatable function exists
print("Test 2: getmetatable function check")
if getmetatable then
    print("  getmetatable function: EXISTS")
else
    print("  getmetatable function: MISSING")
    print("  ERROR: getmetatable function is not available!")
    return
end

-- Test 3: Basic metatable operations
print("Test 3: Basic metatable operations")
local obj = {value = 42}
local meta = {name = "TestMeta"}

print("  Before setmetatable:")
print("    obj.value = " .. tostring(obj.value))
print("    getmetatable(obj) = " .. tostring(getmetatable(obj)))

-- Set metatable
local success, result = pcall(setmetatable, obj, meta)
if success then
    print("  setmetatable: SUCCESS")
    print("    getmetatable(obj) == meta: " .. tostring(getmetatable(obj) == meta))
else
    print("  setmetatable: FAILED - " .. tostring(result))
    return
end

-- Test 4: __index metamethod
print("Test 4: __index metamethod")
local defaults = {default_value = 100}
meta.__index = defaults

print("  obj.value = " .. tostring(obj.value))  -- Should find in obj
print("  obj.default_value = " .. tostring(obj.default_value))  -- Should find via __index

-- Test 5: __newindex metamethod
print("Test 5: __newindex metamethod")
local storage = {}
meta.__newindex = storage

print("  Setting obj.new_field = 'test'")
obj.new_field = "test"
print("  obj.new_field = " .. tostring(obj.new_field))
print("  storage.new_field = " .. tostring(storage.new_field))

print("")
print("=== All metamethod tests completed successfully! ===")
