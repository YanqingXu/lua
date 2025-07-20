-- Test if the issue is file size or parsing complexity
print("=== File Size Issue Test ===")

-- Create a large file with many variables and operations
local test_count = 0
local passed_count = 0
local failed_count = 0

-- Test helper function
local function test_assert(condition, test_name, expected, actual)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        failed_count = failed_count + 1
        local msg = "[Failed] " .. test_name .. " - Failed"
        if expected and actual then
            msg = msg .. " (Expected: " .. tostring(expected) .. ", Actual: " .. tostring(actual) .. ")"
        end
        print(msg)
        return false
    end
end

-- Create many variables to simulate file complexity
local var1 = 1
local var2 = 2
local var3 = 3
local var4 = 4
local var5 = 5
local var6 = 6
local var7 = 7
local var8 = 8
local var9 = 9
local var10 = 10

-- Create many functions
function func1() return 1 end
function func2() return 2 end
function func3() return 3 end
function func4() return 4 end
function func5() return 5 end

-- Create many tables
local table1 = {a = 1, b = 2}
local table2 = {c = 3, d = 4}
local table3 = {e = 5, f = 6}
local table4 = {g = 7, h = 8}
local table5 = {i = 9, j = 10}

-- Perform many operations
test_assert(var1 + var2 == 3, "Addition test 1", 3, var1 + var2)
test_assert(var3 * var4 == 12, "Multiplication test 1", 12, var3 * var4)
test_assert(var5 - var6 == -1, "Subtraction test 1", -1, var5 - var6)
test_assert(var7 / var8 == 0.875, "Division test 1", 0.875, var7 / var8)

test_assert(func1() == 1, "Function test 1", 1, func1())
test_assert(func2() == 2, "Function test 2", 2, func2())
test_assert(func3() == 3, "Function test 3", 3, func3())

test_assert(table1.a == 1, "Table test 1", 1, table1.a)
test_assert(table2.c == 3, "Table test 2", 3, table2.c)
test_assert(table3.e == 5, "Table test 3", 5, table3.e)

-- Add more complexity with loops
local sum = 0
for i = 1, 10 do
    sum = sum + i
end
test_assert(sum == 55, "Loop test", 55, sum)

-- Add string operations
local str1 = "Hello"
local str2 = "World"
local str3 = str1 .. " " .. str2
test_assert(str3 == "Hello World", "String concatenation", "Hello World", str3)

print("=== File Size Issue Test Results ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("Success Rate: " .. (passed_count / test_count * 100) .. "%")
print("File size test completed successfully!")
print("=== End File Size Issue Test ===")
