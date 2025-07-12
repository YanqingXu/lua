-- Test basic metatable functionality
print("=== Basic Metatable Test ===")

-- Test 1: Create table and metatable
local t = {existing = "value"}
local mt = {fallback = "from metatable"}

print("Created table t with t.existing = " .. t.existing)
print("Created metatable mt")

-- Test 2: Set metatable
setmetatable(t, mt)
print("Set metatable for t")

-- Test 3: Test getmetatable
local retrieved = getmetatable(t)
print("getmetatable(t) == mt: " .. tostring(retrieved == mt))

-- Test 4: Test __index with table
mt.__index = mt
print("Set mt.__index = mt")

-- Try to access non-existent key
print("t.fallback = " .. tostring(t.fallback))

print("=== Basic metatable test completed ===")
