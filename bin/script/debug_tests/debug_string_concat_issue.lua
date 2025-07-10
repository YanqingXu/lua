-- Debug string concatenation issue
local i = 2

print("=== Testing the exact pattern ===")
print("  " .. i .. "^2 = " .. (i * i))

print("")
print("=== Breaking it down ===")
local part1 = "  " .. i
print("part1 = '" .. part1 .. "'")

local part2 = part1 .. "^2 = "
print("part2 = '" .. part2 .. "'")

local part3 = i * i
print("part3 = " .. part3)

local result = part2 .. part3
print("result = '" .. result .. "'")

print("")
print("=== Testing with parentheses ===")
print(("  " .. i) .. ("^2 = " .. (i * i)))

print("")
print("=== Testing different groupings ===")
print("Test 1: " .. (("  " .. i) .. "^2 = ") .. (i * i))
print("Test 2: " .. "  " .. (i .. ("^2 = " .. (i * i))))

print("")
print("=== Testing operator precedence ===")
print("a" .. "b" .. "c")
print("1" .. 2 .. "3")
print("x" .. 5 + 3 .. "y")
print("x" .. (5 + 3) .. "y")
