-- Debug table literal initialization
print("=== Debug Table Literal ===")

-- Test 1: Simple table literal
print("Test 1: Simple table literal")
local t = {key = "value"}
print("Created table with literal")

-- Test 2: Check if table exists
print("Test 2: Check table")
if t == nil then
    print("ERROR: Table is nil!")
else
    print("SUCCESS: Table exists")
end

-- Test 3: Check field access
print("Test 3: Check field")
local val = t.key
if val == nil then
    print("ERROR: Field is nil!")
else
    print("SUCCESS: Field = " .. val)
end

print("=== Debug completed ===")
