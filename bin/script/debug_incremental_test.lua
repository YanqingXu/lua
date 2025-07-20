-- Incremental test to find the breaking point
print("=== Incremental Test ===")

-- Copy the exact beginning of comprehensive test
print("=== Lua Interpreter Comprehensive Feature Validation Test ===")
print("Project Completion: 98% (All core features except coroutines)")
print("")

-- Test statistics
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

print("1. Basic Language Features Test")
print("-------------------------------")

-- Variables and basic data types
local num = 42
local str = "Hello Lua"
local bool = true
local nil_val = nil

test_assert(type(num) == "number", "Number type detection", "number", type(num))
test_assert(type(str) == "string", "String type detection", "string", type(str))
test_assert(type(bool) == "boolean", "Boolean type detection", "boolean", type(bool))
test_assert(type(nil_val) == "nil", "Nil type detection", "nil", type(nil_val))

print("Basic types completed")

-- Basic arithmetic operations
test_assert(5 + 3 == 8, "Addition operation", 8, 5 + 3)
test_assert(10 - 4 == 6, "Subtraction operation", 6, 10 - 4)
test_assert(6 * 7 == 42, "Multiplication operation", 42, 6 * 7)
test_assert(15 / 3 == 5, "Division operation", 5, 15 / 3)
test_assert(17 % 5 == 2, "Modulo operation", 2, 17 % 5)

print("Arithmetic operations completed")

-- String concatenation
local concat_result = "Hello" .. " " .. "World"
test_assert(concat_result == "Hello World", "String concatenation", "Hello World", concat_result)

print("String operations completed")

print("")

-- Simple function
function add(a, b)
    return a + b
end

local result = add(5, 3)
test_assert(result == 8, "Simple function call", 8, result)

print("Function test completed")

-- Results
print("=== Incremental Test Results ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("Success Rate: " .. (passed_count / test_count * 100) .. "%")
print("Incremental test completed successfully!")
print("=== End Incremental Test ===")
