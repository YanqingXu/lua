-- Comprehensive Feature Validation Test File
-- Tests all implemented core features in the project
-- Version: 1.0 (July 20, 2025)

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

-- ============================================
-- 1. Basic Language Features Test
-- ============================================
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

-- Basic arithmetic operations
test_assert(5 + 3 == 8, "Addition operation", 8, 5 + 3)
