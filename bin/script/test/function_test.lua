-- Function test
print("=== Function Test ===")

-- Test 1: Check if functions exist
print("Test 1: Check function existence")
print("type(getmetatable) = " .. type(getmetatable))
print("type(setmetatable) = " .. type(setmetatable))

-- Test 2: Check debug functions
print("")
print("Test 2: Check debug functions")
print("type(debug) = " .. type(debug))
if type(debug) == "table" then
    print("type(debug.getmetatable) = " .. type(debug.getmetatable))
    print("type(debug.setmetatable) = " .. type(debug.setmetatable))
end

-- Test 3: Simple table test
print("")
print("Test 3: Simple table test")
local t = {}
print("Created table t")

-- Test 4: Test setmetatable
print("")
print("Test 4: Test setmetatable")
local mt = {}
print("Created metatable mt")

local result = setmetatable(t, mt)
print("setmetatable returned: " .. tostring(result))

print("=== Function test completed ===")
