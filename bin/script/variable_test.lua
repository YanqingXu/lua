-- Test variable definition and access
print("=== Variable Test ===")

-- Test 1: Local variable in same scope
print("Test 1: Local variable in same scope")
local a = 10
print("a =", a)

-- Test 2: Local variable used later
print("Test 2: Local variable used later")
local b = 20
local c = b + 5
print("b =", b)
print("c =", c)

-- Test 3: Global variable
print("Test 3: Global variable")
global_var = 30
print("global_var =", global_var)

print("=== Variable test completed ===")
