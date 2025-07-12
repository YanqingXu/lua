-- Test metatable functions
print("=== Metatable Functions Test ===")

-- Test 1: Basic table creation
print("Test 1: Basic table creation")
local t = {}
t.x = 10
print("Created table t with t.x = " .. t.x)

-- Test 2: Create metatable
print("")
print("Test 2: Create metatable")
local mt = {}
print("Created metatable: " .. tostring(mt))

-- Test 3: Test getmetatable on table without metatable
print("")
print("Test 3: getmetatable on table without metatable")
local result = getmetatable(t)
print("getmetatable(t) = " .. tostring(result))

-- Test 4: Set metatable
print("")
print("Test 4: Set metatable")
local success = setmetatable(t, mt)
print("setmetatable(t, mt) returned: " .. tostring(success))

-- Test 5: Get metatable after setting
print("")
print("Test 5: Get metatable after setting")
local retrieved_mt = getmetatable(t)
print("getmetatable(t) = " .. tostring(retrieved_mt))
print("mt == retrieved_mt: " .. tostring(mt == retrieved_mt))

-- Test 6: Remove metatable
print("")
print("Test 6: Remove metatable")
setmetatable(t, nil)
local after_removal = getmetatable(t)
print("After setmetatable(t, nil), getmetatable(t) = " .. tostring(after_removal))

print("")
print("=== Metatable functions test completed ===")
