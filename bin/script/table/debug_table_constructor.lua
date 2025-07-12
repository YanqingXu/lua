-- Debug table constructor step by step
print("=== Debug Table Constructor ===")

-- Test 1: Empty table
print("Test 1: Empty table")
local t1 = {}
print("Created empty table")

-- Test 2: Single key-value pair
print("Test 2: Single key-value pair")
local t2 = {a = 1}
print("Created table with single pair")
print("t2.a = " .. tostring(t2.a))

-- Test 3: Manual assignment
print("Test 3: Manual assignment")
local t3 = {}
t3.a = 1
print("t3.a = " .. tostring(t3.a))

print("=== Debug completed ===")
