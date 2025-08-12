-- __index metamethod test
print("=== __index Test ===")

-- Test 1: __index with table (should work)
print("Test 1: __index with table")
local defaults = {x = 100, y = 200}
local config = {}

local mt = {__index = defaults}
setmetatable(config, mt)

print("config.x =", config.x)
print("config.y =", config.y)

-- Test 2: Direct table access (should work)
print("\nTest 2: Direct table access")
local direct = {a = 10, b = 20}
print("direct.a =", direct.a)
print("direct.b =", direct.b)

-- Test 3: Missing key (should return nil)
print("\nTest 3: Missing key")
print("config.missing =", config.missing)

print("\n=== Test completed ===")
