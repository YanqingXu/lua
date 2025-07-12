-- Ultra basic test
print("=== Ultra Basic Test ===")

-- Test 1: Variable assignment
print("Test 1: Variable assignment")
local x = 10
local y = "hello"
print("x =", x)
print("y =", y)

-- Test 2: Empty table creation
print("Test 2: Empty table creation")
local t = {}
print("Created empty table t")

-- Test 3: Simple field assignment
print("Test 3: Simple field assignment")
t.a = 5
print("Set t.a = 5")
print("t.a =", t.a)

-- Test 4: String field assignment
print("Test 4: String field assignment")  
t.b = "world"
print("Set t.b = world")
print("t.b =", t.b)

-- Test 5: Multiple assignments
print("Test 5: Multiple assignments")
t.c = 15
t.d = "test"
print("Set t.c = 15, t.d = test")
print("t.c =", t.c)
print("t.d =", t.d)

print("=== Ultra basic test completed ===")
