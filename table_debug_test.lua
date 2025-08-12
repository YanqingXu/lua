-- Debug test for table operations
print("=== Table Debug Test ===")

-- Test 1: Simple table creation
print("Test 1: Table creation")
local t = {}
print("Created table t")

-- Test 2: Simple assignment
print("Test 2: Table assignment")
t["key"] = "value"
print("Assigned t['key'] = 'value'")

-- Test 3: Simple access
print("Test 3: Table access")
local result = t["key"]
print("Retrieved t['key'] =", result)

-- Test 4: Dot notation
print("Test 4: Dot notation")
t.name = "test"
print("Assigned t.name = 'test'")
print("Retrieved t.name =", t.name)

print("=== Table Debug Test Completed ===")
