-- Table literal test
print("=== Table Literal Test ===")

-- Test simple table literal
print("Test 1: Simple table literal")
local t1 = {1, 2, 3}
print("t1[1] =", t1[1])
print("t1[2] =", t1[2])
print("t1[3] =", t1[3])

-- Test key-value table literal
print("Test 2: Key-value table literal")
local t2 = {a = 1, b = 2}
print("t2.a =", t2.a)
print("t2.b =", t2.b)

-- Test mixed table literal
print("Test 3: Mixed table literal")
local t3 = {10, x = 20, 30}
print("t3[1] =", t3[1])
print("t3.x =", t3.x)
print("t3[2] =", t3[2])

-- Test string keys
print("Test 4: String keys")
local t4 = {name = "John", age = 25}
print("t4.name =", t4.name)
print("t4.age =", t4.age)

-- Compare with dynamic assignment
print("Test 5: Dynamic assignment comparison")
local t5 = {}
t5.name = "John"
t5.age = 25
print("t5.name =", t5.name)
print("t5.age =", t5.age)

print("=== Table literal test completed ===")
