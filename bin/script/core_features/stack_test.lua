-- Test stack and register allocation
print("=== Stack Test ===")

-- Test 1: Multiple local variables
print("Test 1: Multiple local variables")
local a = 10
local b = 20
local c = 30

print("a =", a)
print("b =", b) 
print("c =", c)

-- Test 2: Local variables in different order
print("Test 2: Variables in different order")
print("c =", c)
print("a =", a)
print("b =", b)

print("=== Stack test completed ===")
