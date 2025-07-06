-- Debug exact issue
local i = 1

print("=== The problematic expression ===")
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
print("=== Testing right associativity ===")
-- "  " .. i .. "^2 = " .. (i * i)
-- Should be: "  " .. (i .. ("^2 = " .. (i * i)))
-- Which is: "  " .. (1 .. ("^2 = " .. 1))
-- Which is: "  " .. (1 .. "^2 = 1")
-- Which is: "  " .. "1^2 = 1"
-- Which is: "  1^2 = 1"

print("Manual right associative:")
print("  " .. (i .. ("^2 = " .. (i * i))))

print("")
print("=== Testing different groupings ===")
print("Test 1: " .. (("  " .. i) .. ("^2 = " .. (i * i))))
print("Test 2: " .. ("  " .. (i .. ("^2 = " .. (i * i)))))
