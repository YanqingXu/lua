-- Step-by-step debug of comprehensive test
print("=== Step-by-Step Comprehensive Test Debug ===")

-- Step 1: Basic print and variables
print("Step 1: Basic operations")
local test_count = 0
local passed_count = 0
local failed_count = 0

print("Variables created successfully")

-- Step 2: Test helper function
print("Step 2: Creating test helper function")

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

print("Test helper function created successfully")

-- Step 3: Basic type tests
print("Step 3: Basic type tests")

local num = 42
local str = "Hello Lua"
local bool = true
local nil_val = nil

print("Test variables created")

test_assert(type(num) == "number", "Number type detection", "number", type(num))
print("Number type test completed")

test_assert(type(str) == "string", "String type detection", "string", type(str))
print("String type test completed")

test_assert(type(bool) == "boolean", "Boolean type detection", "boolean", type(bool))
print("Boolean type test completed")

test_assert(type(nil_val) == "nil", "Nil type detection", "nil", type(nil_val))
print("Nil type test completed")

-- Step 4: Basic arithmetic
print("Step 4: Basic arithmetic tests")

test_assert(5 + 3 == 8, "Addition operation", 8, 5 + 3)
print("Addition test completed")

test_assert(10 - 4 == 6, "Subtraction operation", 6, 10 - 4)
print("Subtraction test completed")

test_assert(3 * 7 == 21, "Multiplication operation", 21, 3 * 7)
print("Multiplication test completed")

-- Summary
print("=== Step-by-Step Test Summary ===")
print("Total tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("All steps completed successfully!")
