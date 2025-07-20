-- Basic Language Features Test
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
test_assert(type(42) == "number", "Number type detection")
test_assert(type("Hello") == "string", "String type detection")
test_assert(type(true) == "boolean", "Boolean type detection")
test_assert(type(nil) == "nil", "Nil type detection")

-- Arithmetic Operations
print("Testing arithmetic operations...")
test_assert(5 + 3 == 8, "Addition operation")
test_assert(10 - 4 == 6, "Subtraction operation")
test_assert(6 * 7 == 42, "Multiplication operation")
test_assert(15 / 3 == 5, "Division operation")
test_assert(17 % 5 == 2, "Modulo operation")

-- String Operations
print("Testing string operations...")
test_assert("Hello" .. " World" == "Hello World", "String concatenation")

-- Comparison Operations
print("Testing comparison operations...")
test_assert(5 > 3, "Greater than comparison")
test_assert(3 < 5, "Less than comparison")
test_assert(5 >= 5, "Greater than or equal comparison")
test_assert(3 <= 5, "Less than or equal comparison")
test_assert(5 == 5, "Equality comparison")
test_assert(5 ~= 3, "Inequality comparison")

-- Logical Operations
print("Testing logical operations...")
test_assert(true and true, "Logical AND (true)")
test_assert(not (true and false), "Logical AND (false)")
test_assert(true or false, "Logical OR (true)")
test_assert(not (false or false), "Logical OR (false)")
test_assert(not false, "Logical NOT")

-- Variable Assignment
print("Testing variable assignment...")
local x = 10
local y = 20
test_assert(x == 10, "Local variable assignment")
test_assert(y == 20, "Multiple variable assignment")

-- Summary
print("")
print("=== Basic Features Test Summary ===")
print("Total tests: " .. test_count)
print("Passed tests: " .. passed_count)
print("Failed tests: " .. (test_count - passed_count))

if passed_count == test_count then
    print("✅ All basic features working correctly!")
else
    print("❌ Some basic features failed")
end

print("=== Basic Features Test Completed ===")
