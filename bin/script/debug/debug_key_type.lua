-- Key type analysis test
print("=== Key Type Analysis ===")

print("Test 1: String key direct")
local t1 = {}
t1["x"] = 42
print("t1['x'] =", t1["x"])

print("Test 2: Dynamic string assignment")
local t2 = {}
local key = "x"
t2[key] = 42
print("t2[key] =", t2[key])
print("t2['x'] =", t2["x"])

print("Test 3: Member access")
local t3 = {}
t3.x = 42
print("t3.x =", t3.x)
print("t3['x'] =", t3["x"])

print("=== Test completed ===")
