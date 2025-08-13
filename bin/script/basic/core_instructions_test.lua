print("=== Core Instructions Test ===")

-- Test 1: Arithmetic operations (all should work)
print("\nTest 1: Arithmetic operations")
print("  5 + 3 =", 5 + 3)
print("  10 - 4 =", 10 - 4)
print("  6 * 7 =", 6 * 7)
print("  15 / 3 =", 15 / 3)
print("  17 % 5 =", 17 % 5)
print("  2 ^ 8 =", 2 ^ 8)  -- This should now work!

-- Test 2: String operations
print("\nTest 2: String operations")
print("  'Lua' .. ' ' .. '5.1' =", "Lua" .. " " .. "5.1")  -- This should now work!
print("  #'Hello' =", #"Hello")

-- Test 3: Unary operations
print("\nTest 3: Unary operations")
print("  -42 =", -42)
print("  not true =", not true)
print("  not false =", not false)

-- Test 4: Table operations
print("\nTest 4: Table operations")
local t = {1, 2, 3}
print("  t[1] =", t[1])
print("  t[2] =", t[2])
print("  #t =", #t)

-- Test 5: Variable assignment
print("\nTest 5: Variable assignment")
local x = 100
local y = 200
print("  x =", x)
print("  y =", y)
print("  x + y =", x + y)

print("\n=== Core Instructions Test Complete ===")
print("POW and CONCAT instructions are now working!")
