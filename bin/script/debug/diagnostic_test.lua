-- Detailed metamethod diagnostic test
print("=== Metamethod Diagnostic Test ===")

-- Test basic Lua functionality first
print("Test 1: Basic Lua operations")
local t = {x = 10, y = 20}
print("  t.x = " .. tostring(t.x))
print("  t.y = " .. tostring(t.y))

-- Check function availability
print("Test 2: Function availability check")
print("  type(setmetatable) = " .. type(setmetatable))
print("  type(getmetatable) = " .. type(getmetatable))

-- Test with very simple case
print("Test 3: Simple metatable test")
local obj = {}
local meta = {}

print("  Created obj and meta tables")
print("  type(obj) = " .. type(obj))
print("  type(meta) = " .. type(meta))

-- Try to set metatable with error handling
print("Test 4: setmetatable with error handling")
local status, error_msg = pcall(function()
    setmetatable(obj, meta)
end)

if status then
    print("  setmetatable: SUCCESS")
    local mt = getmetatable(obj)
    print("  getmetatable result: " .. tostring(mt))
    print("  mt == meta: " .. tostring(mt == meta))
else
    print("  setmetatable: FAILED")
    print("  Error: " .. tostring(error_msg))
end

print("")
print("=== Diagnostic completed ===")
