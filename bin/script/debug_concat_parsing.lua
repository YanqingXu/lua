-- Debug concatenation parsing
local i = 2

print("=== Testing step by step ===")

print("Step 1: Simple concat")
print("a" .. "b")

print("")
print("Step 2: Variable concat")
print("a" .. i)

print("")
print("Step 3: Two variables")
print(i .. i)

print("")
print("Step 4: Three parts")
print("a" .. i .. "b")

print("")
print("Step 5: Four parts")
print("a" .. i .. "b" .. i)

print("")
print("Step 6: The problem pattern")
print("  " .. i .. "^2 = " .. 4)
