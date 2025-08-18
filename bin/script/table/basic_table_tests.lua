-- Basic table operation tests
-- Tests for basic operations like creation, access, and modification

print("=== Basic table operation tests ===")

-- Test 1: Create empty table
print("Test 1: Create empty table")
local emptyTable = {}
print("✓ Empty table created successfully")

-- Test 2: Array-style table creation
print("\nTest 2: Array-style table creation")
local array = {1, 2, 3, 4, 5}
print("  array[1] =", array[1])
print("  array[3] =", array[3])
print("  array[5] =", array[5])

-- Test 3: Dictionary-style table creation
print("\nTest 3: Dictionary-style table creation")
local dict = {
    name = "Zhang San",
    age = 25,
    city = "Beijing"
}
print("  dict.name =", dict.name)
print("  dict.age =", dict.age)
print("  dict.city =", dict.city)

-- Test 4: Mixed index table
print("\nTest 4: Mixed index table")
local mixed = {
    "First element",
    "Second element",
    name = "Test table",
    [10] = "Position 10"
}
print("  mixed[1] =", mixed[1])
print("  mixed[2] =", mixed[2])
print("  mixed.name =", mixed.name)
print("  mixed[10] =", mixed[10])

-- Test 5: Dynamically add elements
print("\nTest 5: Dynamically add elements")
local dynamic = {}
dynamic[1] = "First"
dynamic["key"] = "Value"
dynamic.field = "Field value"
print("  dynamic[1] =", dynamic[1])
print("  dynamic['key'] =", dynamic["key"])
print("  dynamic.field =", dynamic.field)

-- Test 6: Modify table elements
print("\nTest 6: Modify table elements")
local modifiable = {a = 1, b = 2, c = 3}
print("  Before: a=" .. modifiable.a .. ", b=" .. modifiable.b)
modifiable.a = 10
modifiable.b = 20
print("  After: a=" .. modifiable.a .. ", b=" .. modifiable.b)

-- Test 7: Delete elements (set to nil)
print("\nTest 7: Delete elements")
local deletable = {x = 100, y = 200, z = 300}
print("  Before deletion: x=" .. tostring(deletable.x) .. ", y=" .. tostring(deletable.y))
deletable.x = nil
print("  After deletion: x=" .. tostring(deletable.x) .. ", y=" .. tostring(deletable.y))

-- Test 8: Access non-existent key
print("\nTest 8: Access non-existent key")
local testTable = {a = 1}
local nonExistent = testTable.nonExistent
print("  Non-existent key value:", tostring(nonExistent))
if nonExistent == nil then
    print("✓ Non-existent key correctly returned nil")
else
    print("✗ Non-existent key returned non-nil value")
end

-- Test 9: Number keys and string keys
print("\nTest 9: Number keys and string keys")
local keyTest = {}
keyTest[1] = "Number key 1"
keyTest["1"] = "String key 1"
print("  keyTest[1] =", keyTest[1])
print("  keyTest['1'] =", keyTest["1"])

-- Test 10: Length operator for tables
print("\nTest 10: Length operator for tables")
local lengthTest = {10, 20, 30, 40}
print("  #lengthTest =", #lengthTest)

local sparseArray = {[1] = "a", [3] = "c", [5] = "e"}
print("  #sparseArray =", #sparseArray)

-- Test 11: Nested tables
print("\nTest 11: Nested tables")
local nested = {
    level1 = {
        level2 = {
            value = "Deep value"
        }
    }
}
print("  nested.level1.level2.value =", nested.level1.level2.value)

-- Test 12: Tables as values
print("\nTest 12: Tables as values")
local table1 = {a = 1}
local table2 = {b = 2}
local container = {
    first = table1,
    second = table2
}
print("  container.first.a =", container.first.a)
print("  container.second.b =", container.second.b)

print("\n=== Basic table operation tests completed ===")
