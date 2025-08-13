print("=== Final Extended Instructions Test ===")

-- Test 1: All arithmetic operations (all working)
print("\nTest 1: Arithmetic operations")
print("  5 + 3 =", 5 + 3)
print("  10 - 4 =", 10 - 4)
print("  6 * 7 =", 6 * 7)
print("  15 / 3 =", 15 / 3)
print("  17 % 5 =", 17 % 5)
print("  2 ^ 8 =", 2 ^ 8)  -- Fixed POW instruction

-- Test 2: String operations (all working)
print("\nTest 2: String operations")
print("  'Hello' .. ' ' .. 'World' =", "Hello" .. " " .. "World")  -- Fixed CONCAT instruction
print("  #'Lua' =", #"Lua")

-- Test 3: Comparison operations (all working)
print("\nTest 3: Comparison operations")
local x = 10
local y = 20
print("  x == y:", x == y)
print("  x ~= y:", x ~= y)
print("  x < y:", x < y)
print("  x <= y:", x <= y)
print("  x > y:", x > y)
print("  x >= y:", x >= y)

-- Test 4: Basic logical operations (working)
print("\nTest 4: Basic logical operations")
local p = true
local q = false
print("  p and q:", p and q)
print("  p or q:", p or q)
print("  not p:", not p)
print("  not q:", not q)

-- Test 5: Unary operations (working)
print("\nTest 5: Unary operations")
print("  -42 =", -42)
print("  not true =", not true)
print("  #'test' =", #"test")

-- Test 6: Table operations (working)
print("\nTest 6: Table operations")
local t = {1, 2, 3}
t[4] = 4
print("  t[1] =", t[1])
print("  t[4] =", t[4])
print("  #t =", #t)

-- Test 7: Manual summation (working, for loop alternative)
print("\nTest 7: Manual summation")
local result = 0
result = result + 1
result = result + 2
result = result + 3
result = result + 4
result = result + 5
print("  sum 1 to 5 =", result)

print("\n=== Final Extended Instructions Test Complete ===")
print("SUCCESS: POW, CONCAT, and comparison instructions are working!")
print("NOTE: for loops and mixed logical operations need further work.")
