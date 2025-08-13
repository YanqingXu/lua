print("=== Extended Instructions Test (Task 2.2) ===")

-- Test 1: Arithmetic operations
print("\nTest 1: Arithmetic operations")
local a = 10
local b = 5
print("  a + b =", a + b)
print("  a - b =", a - b)
print("  a * b =", a * b)
print("  a / b =", a / b)
print("  a % b =", a % b)
print("  a ^ b =", a ^ b)

-- Test 2: Comparison operations
print("\nTest 2: Comparison operations")
local x = 10
local y = 20
print("  x == y:", x == y)
print("  x ~= y:", x ~= y)
print("  x < y:", x < y)
print("  x <= y:", x <= y)
print("  x > y:", x > y)
print("  x >= y:", x >= y)

-- Test 3: Logical operations
print("\nTest 3: Logical operations")
local p = true
local q = false
print("  p and q:", p and q)
print("  p or q:", p or q)
print("  not p:", not p)
print("  not q:", not q)

-- Test 4: String operations
print("\nTest 4: String operations")
local s1 = "Hello"
local s2 = "World"
print("  s1 .. s2 =", s1 .. s2)
print("  #s1 =", #s1)

-- Test 5: Unary operations
print("\nTest 5: Unary operations")
local num = 42
print("  -num =", -num)
print("  #'test' =", #"test")

-- Test 6: Table operations with extended instructions
print("\nTest 6: Table operations")
local t = {1, 2, 3}
t[4] = 4
print("  t[1] =", t[1])
print("  t[4] =", t[4])
print("  #t =", #t)

-- Test 7: Function calls with multiple arguments
print("\nTest 7: Function calls")
local function add_three(a, b, c)
    return a + b + c
end
print("  add_three(1, 2, 3) =", add_three(1, 2, 3))

-- Test 8: Variable argument functions
print("\nTest 8: Variable arguments")
local function sum(...)
    local total = 0
    local args = {...}
    for i = 1, #args do
        total = total + args[i]
    end
    return total
end
print("  sum(1, 2, 3, 4, 5) =", sum(1, 2, 3, 4, 5))

-- Test 9: Control flow
print("\nTest 9: Control flow")
local result = 0
for i = 1, 5 do
    result = result + i
end
print("  sum 1 to 5 =", result)

-- Test 10: Conditional expressions
print("\nTest 10: Conditional expressions")
local val = 10
local msg = val > 5 and "big" or "small"
print("  val > 5 ? 'big' : 'small' =", msg)

print("\n=== Extended Instructions Test Complete ===")
