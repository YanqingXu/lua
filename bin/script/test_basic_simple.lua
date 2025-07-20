-- Basic Language Features Test (Simplified)
-- Tests fundamental Lua language features

print("=== Basic Language Features Test ===")

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

-- Data Type Tests
print("Testing data types...")

local num = 42
local str = "Hello"
local bool = true
local nil_val = nil

test_assert(type(num) == "number", "Number type detection")
test_assert(type(str) == "string", "String type detection")
test_assert(type(bool) == "boolean", "Boolean type detection")
test_assert(type(nil_val) == "nil", "Nil type detection")

-- Arithmetic Operations
print("Testing arithmetic operations...")

local a = 5
local b = 3
local add_result = a + b
test_assert(add_result == 8, "Addition operation")

local c = 10
local d = 4
local sub_result = c - d
test_assert(sub_result == 6, "Subtraction operation")

local e = 6
local f = 7
local mul_result = e * f
test_assert(mul_result == 42, "Multiplication operation")

local g = 15
local h = 3
local div_result = g / h
test_assert(div_result == 5, "Division operation")

local i = 17
local j = 5
local mod_result = i % j
test_assert(mod_result == 2, "Modulo operation")

-- String Operations
print("Testing string operations...")

local str1 = "Hello"
local str2 = " World"
local concat_result = str1 .. str2
test_assert(concat_result == "Hello World", "String concatenation")

-- Variable Assignment
print("Testing variable assignment...")

local x = 10
local y = 20
test_assert(x == 10, "Local variable assignment")
test_assert(y == 20, "Multiple variable assignment")

-- Summary
print("")
print("=== Basic Features Test Summary ===")
print("Total tests: " .. tostring(test_count))
print("Passed tests: " .. tostring(passed_count))

local failed_count = test_count - passed_count
print("Failed tests: " .. tostring(failed_count))

if passed_count == test_count then
    print("All basic features working correctly!")
else
    print("Some basic features failed")
end

print("=== Basic Features Test Completed ===")
