-- Debug logical or operator
print("=== Debug Logical OR ===")

-- Test 1: true or false
local result1 = true or false
print("true or false =", result1)
print("Expected: true")

-- Test 2: false or true
local result2 = false or true
print("false or true =", result2)
print("Expected: true")

-- Test 3: false or false
local result3 = false or false
print("false or false =", result3)
print("Expected: false")

-- Test 4: 5 or -2 (numbers)
local c = 5
local d = -2
local result4 = c > 0 or d > 0
print("5 > 0 or -2 > 0 =", result4)
print("Expected: true")

-- Test 5: -5 or 2 (numbers)
local e = -5
local f = 2
local result5 = e > 0 or f > 0
print("-5 > 0 or 2 > 0 =", result5)
print("Expected: true")

-- Test 6: -5 or -2 (numbers)
local g = -5
local h = -2
local result6 = g > 0 or h > 0
print("-5 > 0 or -2 > 0 =", result6)
print("Expected: false")

print("=== Debug End ===")
