-- Simple Comprehensive Test (avoiding complex expressions)
print("=== Simple Comprehensive Lua Interpreter Test ===")

-- Test statistics
local test_count = 0
local passed_count = 0
local failed_count = 0

-- Test helper function
local function test_assert(condition, test_name)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name)
        return true
    else
        failed_count = failed_count + 1
        print("[Failed] " .. test_name)
        return false
    end
end

-- Basic tests
print("1. Basic Language Features")
print("--------------------------")

local num = 42
local str = "Hello"
local bool = true

test_assert(type(num) == "number", "Number type detection")
test_assert(type(str) == "string", "String type detection")
test_assert(type(bool) == "boolean", "Boolean type detection")

test_assert(5 + 3 == 8, "Addition operation")
test_assert(10 - 4 == 6, "Subtraction operation")
test_assert(3 * 7 == 21, "Multiplication operation")

-- String operations
test_assert("Hello" .. " World" == "Hello World", "String concatenation")

-- Logical operations
test_assert(true and false == false, "Logical AND")
test_assert(true or false == true, "Logical OR")

print("")

-- Function tests
print("2. Function Tests")
print("-----------------")

function add(a, b)
    return a + b
end

local result = add(5, 3)
test_assert(result == 8, "Simple function call")

print("")

-- Table tests
print("3. Table Tests")
print("--------------")

local table1 = {1, 2, 3, 4, 5}
test_assert(table1[1] == 1, "Table indexing")
test_assert(table1[5] == 5, "Table last element")

-- Table length
local length = #table1
test_assert(length == 5, "Table length operator")

print("")

-- Control flow tests
print("4. Control Flow Tests")
print("---------------------")

-- if-then-else
local if_result = 0
if 5 > 3 then
    if_result = 1
else
    if_result = 2
end
test_assert(if_result == 1, "if-then-else statement")

-- for loop
local for_sum = 0
for i = 1, 5 do
    for_sum = for_sum + i
end
test_assert(for_sum == 15, "for loop")

-- break statement
local break_sum = 0
for i = 1, 10 do
    if i > 5 then
        break
    end
    break_sum = break_sum + i
end
test_assert(break_sum == 15, "break statement")

print("")

-- Results
print("===================")
print("FINAL RESULTS")
print("===================")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)

if failed_count == 0 then
    print("ALL TESTS PASSED!")
else
    print("Some tests failed.")
end

print("Test completed successfully!")
