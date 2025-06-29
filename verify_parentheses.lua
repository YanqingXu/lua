-- Verify parentheses calculation
local i = 2

print("=== Testing parentheses calculation ===")
print("i = " .. i)

print("")
print("Testing (i * i):")
local result = (i * i)
print("result = " .. result)

print("")
print("Testing in concatenation:")
print("Value: " .. (i * i))

print("")
print("Testing the full expression:")
print("  " .. i .. "^2 = " .. (i * i))

print("")
print("Testing without parentheses:")
print("  " .. i .. "^2 = " .. i * i)
