-- Test parentheses in concatenation
local i = 2

print("=== Without parentheses ===")
print("  " .. i .. "^2 = " .. i * i)

print("")
print("=== With parentheses ===")
print("  " .. i .. "^2 = " .. (i * i))

print("")
print("=== Different parentheses ===")
print(("  " .. i) .. ("^2 = " .. (i * i)))

print("")
print("=== Step by step ===")
local mult = i * i
print("mult = " .. mult)
print("  " .. i .. "^2 = " .. mult)
