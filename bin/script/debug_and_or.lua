-- Debug AND and OR operators
print("=== AND/OR Debug ===")

-- Test AND (working)
local and_result1 = true and false
print("true and false =", and_result1)
print("Expected: false")

local and_result2 = true and true
print("true and true =", and_result2)
print("Expected: true")

-- Test OR (not working)
local or_result1 = true or false
print("true or false =", or_result1)
print("Expected: true")

local or_result2 = false or true
print("false or true =", or_result2)
print("Expected: true")

print("=== End ===")
