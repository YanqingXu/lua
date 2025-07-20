-- Test if compiler fix works
print("=== Compiler Fix Test ===")

-- Test complex expression that might trigger constant folding
local total = 10
local passed = 8
local rate = (passed / total) * 100

print("Rate calculation:", rate)
print("Expected: 80, Actual:", rate)

-- Test another complex expression
local a = 5
local b = 3
local c = 2
local result = a + b * c - 1

print("Complex expression result:", result)
print("Expected: 10, Actual:", result)

print("=== Compiler Fix Test Complete ===")
