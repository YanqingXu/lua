-- Test operator precedence
print("=== Testing precedence ===")
print("2 + 3 * 4 = " .. (2 + 3 * 4))  -- Should be 14
print("2 * 3 + 4 = " .. (2 * 3 + 4))  -- Should be 10

print("")
print("=== Testing concatenation precedence ===")
print("Result: " .. 2 + 3)  -- Should be "Result: 5"
print("Result: " .. (2 + 3))  -- Should be "Result: 5"

print("")
print("=== Testing the problem ===")
local i = 2
print("Direct: " .. i .. " + " .. i)  -- Should be "Direct: 2 + 2"
