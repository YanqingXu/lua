-- Basic table test
print("=== Basic Table Test ===")

-- Test 1: Create empty table
print("Test 1: Create empty table")
local t1 = {}
print("Created empty table t1")

-- Test 2: Add value to table
print("")
print("Test 2: Add value to table")
t1.key = "value"
print("Set t1.key = 'value'")

-- Test 3: Access table value
print("")
print("Test 3: Access table value")
local val = t1.key
print("Retrieved t1.key")
if val == nil then
    print("ERROR: t1.key is nil!")
else
    print("SUCCESS: t1.key = " .. val)
end

-- Test 4: Create table with initial values
print("")
print("Test 4: Create table with initial values")
local t2 = {name = "test", value = 42}
print("Created table t2 with initial values")

-- Test 5: Access initial values
print("")
print("Test 5: Access initial values")
local name = t2.name
local value = t2.value
print("Retrieved values from t2")

if name == nil then
    print("ERROR: t2.name is nil!")
else
    print("SUCCESS: t2.name = " .. name)
end

if value == nil then
    print("ERROR: t2.value is nil!")
else
    print("SUCCESS: t2.value = " .. tostring(value))
end

print("")
print("=== Basic table test completed ===")
