-- Step 1: Basic functionality test
print("=== Step 1: Basic Test ===")

-- Test A: Simple variables
print("Test A: Simple variables")
local x = 5
print("x = " .. tostring(x))

-- Test B: Empty table creation
print("Test B: Empty table creation")
local t = {}
print("Created empty table")

-- Test C: Table assignment
print("Test C: Table assignment")
t.key = "value"
print("Assigned t.key = 'value'")

-- Test D: Table access
print("Test D: Table access")
local val = t.key
print("Retrieved value: " .. tostring(val))

print("=== Step 1 completed ===")
