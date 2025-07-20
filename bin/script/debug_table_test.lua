-- Table test to check table functionality
print("=== Table Debug Test ===")

-- Test helper function
local function test_assert(condition, test_name)
    if condition then
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        print("[Failed] " .. test_name .. " - Failed")
        return false
    end
end

-- Test 1: Empty table creation
local empty_table = {}
test_assert(type(empty_table) == "table", "Empty table creation")

-- Test 2: Table field assignment
local obj = {}
obj.name = "test"
obj.value = 100
test_assert(obj.name == "test", "Table field assignment (string)")
test_assert(obj.value == 100, "Table field assignment (number)")

-- Test 3: Table literal
local literal_table = {x = 10, y = 20}
test_assert(literal_table.x == 10, "Table literal (x)")
test_assert(literal_table.y == 20, "Table literal (y)")

-- Test 4: Array-style table
local array = {1, 2, 3, 4, 5}
test_assert(array[1] == 1, "Array access [1]")
test_assert(array[3] == 3, "Array access [3]")

print("=== Table test completed ===")
