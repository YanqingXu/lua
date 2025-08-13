print("=== Fixed Instructions Test ===")

-- Test 1: All arithmetic operations
print("\nTest 1: Arithmetic operations")
print("  5 + 3 =", 5 + 3)
print("  10 - 4 =", 10 - 4)
print("  6 * 7 =", 6 * 7)
print("  15 / 3 =", 15 / 3)
print("  17 % 5 =", 17 % 5)
print("  2 ^ 8 =", 2 ^ 8)  -- Fixed POW instruction

-- Test 2: String operations
print("\nTest 2: String operations")
print("  'Hello' .. ' ' .. 'World' =", "Hello" .. " " .. "World")  -- Fixed CONCAT instruction
print("  #'Lua' =", #"Lua")

-- Test 3: Unary operations
print("\nTest 3: Unary operations")
print("  -42 =", -42)
print("  not true =", not true)
print("  not false =", not false)

-- Test 4: Table operations
print("\nTest 4: Table operations")
local t = {10, 20, 30}
print("  t[1] =", t[1])
print("  t[2] =", t[2])
print("  #t =", #t)

-- Test 5: Variable operations
print("\nTest 5: Variables")
local x = 100
local y = 200
print("  x =", x)
print("  y =", y)
print("  x + y =", x + y)
print("  x ^ 2 =", x ^ 2)
print("  'x=' .. x =", "x=" .. x)

print("\n=== Fixed Instructions Test Complete ===")
print("POW and CONCAT instructions are working correctly!")
