print("=== Comparison Debug Test ===")

local a = 1
local b = 3

print("a =", a)
print("b =", b)

-- Test each comparison individually
local result1 = (a <= b)
print("a <= b =", result1)

local result2 = (a < b)
print("a < b =", result2)

local result3 = (a >= b)
print("a >= b =", result3)

local result4 = (a > b)
print("a > b =", result4)

local result5 = (a == b)
print("a == b =", result5)

local result6 = (a ~= b)
print("a ~= b =", result6)

print("=== Comparison Debug Test Complete ===")
