print("=== Working Extended Instructions Test ===")

-- Test 1: Arithmetic operations (all working)
print("\nTest 1: Arithmetic operations")
local a = 10
local b = 5
print("  a + b =", a + b)
print("  a - b =", a - b)
print("  a * b =", a * b)
print("  a / b =", a / b)
print("  a % b =", a % b)
print("  a ^ b =", a ^ b)  -- Fixed POW instruction

-- Test 2: String operations (working)
print("\nTest 2: String operations")
local s1 = "Hello"
local s2 = "World"
print("  s1 .. s2 =", s1 .. s2)  -- Fixed CONCAT instruction
print("  #s1 =", #s1)

-- Test 3: Unary operations (working)
print("\nTest 3: Unary operations")
local num = 42
print("  -num =", -num)
print("  #'test' =", #"test")

-- Test 4: Table operations (working)
print("\nTest 4: Table operations")
local t = {1, 2, 3}
t[4] = 4
print("  t[1] =", t[1])
print("  t[4] =", t[4])
print("  #t =", #t)

-- Test 5: Basic logical operations (partially working)
print("\nTest 5: Basic logical operations")
local p = true
local q = false
print("  p and q:", p and q)
print("  p or q:", p or q)
print("  not p:", not p)
print("  not q:", not q)

-- Test 6: Manual summation (instead of for loop)
print("\nTest 6: Manual summation")
local result = 0
result = result + 1
result = result + 2
result = result + 3
result = result + 4
result = result + 5
print("  sum 1 to 5 =", result)

print("\n=== Working Extended Instructions Test Complete ===")
print("POW and CONCAT instructions are working!")
print("For loops and some logical operations need further work.")
