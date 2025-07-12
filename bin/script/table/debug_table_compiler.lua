-- Debug table compiler test
print("=== Debug Table Compiler Test ===")

print("Test 1: Empty table")
local t1 = {}
print("Created empty table:", t1)

print("Test 2: Simple array")
local t2 = {1, 2, 3}
print("Created array:", t2)
print("t2[1] =", t2[1])
print("t2[2] =", t2[2])
print("t2[3] =", t2[3])

print("Test 3: Key-value pairs (problematic)")
local t3 = {x = 10, y = 20}
print("Created table with key-value:", t3)
print("t3.x =", t3.x)
print("t3.y =", t3.y)

print("Test 4: Mixed table")
local t4 = {5, x = 15}
print("Created mixed table:", t4)
print("t4[1] =", t4[1])
print("t4.x =", t4.x)

print("Test 5: String keys")
local t5 = {name = "test", value = 42}
print("Created table with string keys:", t5)
print("t5.name =", t5.name)
print("t5.value =", t5.value)

print("=== Debug test completed ===")
