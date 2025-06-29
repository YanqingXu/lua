-- Debug concatenation order
local i = 2

print("=== Step by step ===")
local part1 = "  " .. i
print("part1 = '  ' .. i = '" .. part1 .. "'")

local part2 = part1 .. "^2 = "
print("part2 = part1 .. '^2 = ' = '" .. part2 .. "'")

local part3 = i * i
print("part3 = i * i = " .. part3)

local final = part2 .. part3
print("final = part2 .. part3 = '" .. final .. "'")

print("")
print("=== Direct expression ===")
print("  " .. i .. "^2 = " .. (i * i))

print("")
print("=== With parentheses ===")
print(("  " .. i) .. ("^2 = " .. (i * i)))
