-- Simple member access test
print("=== Simple Member Access Test ===")

print("Test 1: Member assignment and access")
local t = {}
t.x = 42
print("t.x =", t.x)

print("Test 2: String key access")
print("t['x'] =", t["x"])

print("=== Test completed ===")
