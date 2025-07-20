-- Table Features Test (Simplified)
-- Tests table creation, access, and manipulation

print("=== Table Features Test ===")

local test_count = 0
local passed_count = 0

function test_assert(condition, test_name)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name)
    else
        print("[FAILED] " .. test_name)
    end
end

-- Empty Table Creation
print("Testing empty table creation...")

local empty_table = {}
test_assert(type(empty_table) == "table", "Empty table creation")

-- Table Field Assignment
print("Testing table field assignment...")

local obj = {}
obj.name = "test_object"
obj.value = 100
obj.active = true

test_assert(obj.name == "test_object", "String field assignment")
test_assert(obj.value == 100, "Number field assignment")
test_assert(obj.active == true, "Boolean field assignment")

-- Table Literal Creation
print("Testing table literal creation...")

local point = {x = 10, y = 20, z = 30}
test_assert(point.x == 10, "Table literal x coordinate")
test_assert(point.y == 20, "Table literal y coordinate")
test_assert(point.z == 30, "Table literal z coordinate")

-- Array-style Tables
print("Testing array-style tables...")

local numbers = {1, 2, 3, 4, 5}
test_assert(numbers[1] == 1, "Array access index 1")
test_assert(numbers[3] == 3, "Array access index 3")
test_assert(numbers[5] == 5, "Array access index 5")

-- Dynamic Field Assignment
print("Testing dynamic field assignment...")

local dynamic = {}
dynamic["key1"] = "value1"
dynamic["key2"] = 42
dynamic[100] = "numeric_key"

test_assert(dynamic["key1"] == "value1", "Dynamic string key assignment")
test_assert(dynamic["key2"] == 42, "Dynamic string key with number value")
test_assert(dynamic[100] == "numeric_key", "Dynamic numeric key assignment")

-- Table Modification
print("Testing table modification...")

local modifiable = {a = 1, b = 2}
modifiable.a = 10
modifiable.c = 3

test_assert(modifiable.a == 10, "Table field modification")
test_assert(modifiable.b == 2, "Unchanged table field")
test_assert(modifiable.c == 3, "New table field addition")

-- Table as Function Parameter
print("Testing tables as function parameters...")

function get_table_sum(t)
    return t.a + t.b + t.c
end

local sum_table = {a = 1, b = 2, c = 3}
local sum_result = get_table_sum(sum_table)
test_assert(sum_result == 6, "Table as function parameter")

-- Table Length with # operator
print("Testing table length...")

local length_test = {10, 20, 30, 40}
local length_result = #length_test
test_assert(length_result == 4, "Table length operator")

-- Summary
print("")
print("=== Table Features Test Summary ===")
print("Total tests: " .. tostring(test_count))
print("Passed tests: " .. tostring(passed_count))

local failed_count = test_count - passed_count
print("Failed tests: " .. tostring(failed_count))

if passed_count == test_count then
    print("All table features working correctly!")
else
    print("Some table features failed")
end

print("=== Table Features Test Completed ===")
