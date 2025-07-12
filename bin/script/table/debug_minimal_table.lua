-- Minimal table literal debug test
print("=== Minimal Table Literal Debug ===")

print("Test 1: Single key-value pair")
local t1 = {x = 10}
print("t1.x =", t1.x)

print("Test 2: Two key-value pairs")
local t2 = {x = 10, y = 20}
print("t2.x =", t2.x)
print("t2.y =", t2.y)

print("Test 3: Same but with dynamic assignment")
local t3 = {}
t3.x = 10
t3.y = 20
print("t3.x =", t3.x)
print("t3.y =", t3.y)

print("=== Test completed ===")
