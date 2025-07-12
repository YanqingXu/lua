-- Simple test for constant limits
print("=== Testing Simple Constant Limits ===")

-- Test with manual string constants
local t = {}
t.key001 = "value001"
t.key002 = "value002"
t.key003 = "value003"
-- ... continue adding many keys manually to test limits

print("t.key001 =", t.key001)
print("t.key002 =", t.key002)
print("t.key003 =", t.key003)

-- Test member access with high constant indices
print("Testing member access...")
local result1 = t.key001
local result2 = t.key002
local result3 = t.key003

print("result1 =", result1)
print("result2 =", result2)
print("result3 =", result3)

print("=== Test completed ===")
