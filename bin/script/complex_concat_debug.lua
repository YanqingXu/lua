-- Complex concatenation debug
local i = 2

print("=== Testing different patterns ===")
print("Pattern 1: " .. i .. "x")
print("Pattern 2: " .. i .. "x" .. i)
print("Pattern 3: " .. i .. "^2")
print("Pattern 4: " .. i .. "^2 = ")
print("Pattern 5: " .. i .. "^2 = " .. i)

print("")
print("=== The problematic pattern ===")
print("  " .. i .. "^2 = " .. (i * i))

print("")
print("=== Breaking it down ===")
local step1 = "  " .. i
print("Step 1: '  ' .. i = '" .. step1 .. "'")

local step2 = step1 .. "^2 = "
print("Step 2: step1 .. '^2 = ' = '" .. step2 .. "'")

local step3 = i * i
print("Step 3: i * i = " .. step3)

local step4 = step2 .. step3
print("Step 4: step2 .. step3 = '" .. step4 .. "'")

print("")
print("=== Alternative approach ===")
local result = i * i
print("  " .. i .. "^2 = " .. result)
