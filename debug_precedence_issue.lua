-- Debug precedence issue
local i = 1

print("=== Step by step analysis ===")
print("i = " .. i)
print("i * i = " .. (i * i))

print("")
print("=== The problematic expression ===")
print("  " .. i .. "^2 = " .. (i * i))

print("")
print("=== Manual step by step ===")
local step1 = i * i
print("step1 (i * i) = " .. step1)

local step2 = "  " .. i
print("step2 ('  ' .. i) = '" .. step2 .. "'")

local step3 = step2 .. "^2 = "
print("step3 (step2 .. '^2 = ') = '" .. step3 .. "'")

local step4 = step3 .. step1
print("step4 (step3 .. step1) = '" .. step4 .. "'")

print("")
print("=== Testing simpler cases ===")
print("a" .. "b" .. "c")
print("1" .. "2" .. "3")
print("x" .. i .. "y")
