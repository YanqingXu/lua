-- Standard Library Functions Test
-- Tests built-in Lua standard library functions

print("=== Standard Library Functions Test ===")

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

-- Base Library Functions
print("Testing base library functions...")

local str_42 = tostring(42)
test_assert(str_42 == "42", "tostring function (number)")

local str_true = tostring(true)
test_assert(str_true == "true", "tostring function (boolean)")

local str_nil = tostring(nil)
test_assert(str_nil == "nil", "tostring function (nil)")

local num_123 = tonumber("123")
test_assert(num_123 == 123, "tonumber function (valid string)")

local num_invalid = tonumber("abc")
test_assert(num_invalid == nil, "tonumber function (invalid string)")

local type_num = type(42)
test_assert(type_num == "number", "type function (number)")

local type_str = type("hello")
test_assert(type_str == "string", "type function (string)")

-- Math Library Functions
print("Testing math library functions...")

local abs_result = math.abs(-5)
test_assert(abs_result == 5, "math.abs function (negative)")

local abs_result2 = math.abs(3)
test_assert(abs_result2 == 3, "math.abs function (positive)")

local max_result = math.max(1, 5, 3)
test_assert(max_result == 5, "math.max function")

local max_result2 = math.max(-1, -5, -3)
test_assert(max_result2 == -1, "math.max function (negatives)")

local min_result = math.min(1, 5, 3)
test_assert(min_result == 1, "math.min function")

local min_result2 = math.min(-1, -5, -3)
test_assert(min_result2 == -5, "math.min function (negatives)")

local floor_result = math.floor(3.7)
test_assert(floor_result == 3, "math.floor function")

local floor_result2 = math.floor(-2.3)
test_assert(floor_result2 == -3, "math.floor function (negative)")

local ceil_result = math.ceil(3.2)
test_assert(ceil_result == 4, "math.ceil function")

local ceil_result2 = math.ceil(-2.7)
test_assert(ceil_result2 == -2, "math.ceil function (negative)")

-- String Library Functions
print("Testing string library functions...")

local len_result = string.len("hello")
test_assert(len_result == 5, "string.len function")

local len_result2 = string.len("")
test_assert(len_result2 == 0, "string.len function (empty string)")

local upper_result = string.upper("hello")
test_assert(upper_result == "HELLO", "string.upper function")

local upper_result2 = string.upper("Hello World")
test_assert(upper_result2 == "HELLO WORLD", "string.upper function (mixed case)")

local lower_result = string.lower("WORLD")
test_assert(lower_result == "world", "string.lower function")

local lower_result2 = string.lower("Hello World")
test_assert(lower_result2 == "hello world", "string.lower function (mixed case)")

local sub_result = string.sub("hello", 2, 4)
test_assert(sub_result == "ell", "string.sub function")

local sub_result2 = string.sub("hello", 1, 3)
test_assert(sub_result2 == "hel", "string.sub function (from start)")

local sub_result3 = string.sub("hello", 3)
test_assert(sub_result3 == "llo", "string.sub function (to end)")

-- Math Constants
print("Testing math constants...")

local pi_exists = type(math.pi) == "number"
test_assert(pi_exists, "math.pi constant exists")

local huge_exists = type(math.huge) == "number"
test_assert(huge_exists, "math.huge constant exists")

-- Additional Math Functions
print("Testing additional math functions...")

local sqrt_result = math.sqrt(16)
test_assert(sqrt_result == 4, "math.sqrt function")

local sqrt_result2 = math.sqrt(25)
test_assert(sqrt_result2 == 5, "math.sqrt function (25)")

-- Summary
print("")
print("=== Standard Library Functions Test Summary ===")
print("Total tests: " .. tostring(test_count))
print("Passed tests: " .. tostring(passed_count))

local failed_count = test_count - passed_count
print("Failed tests: " .. tostring(failed_count))

if passed_count == test_count then
    print("All standard library functions working correctly!")
else
    print("Some standard library functions failed")
end

print("=== Standard Library Functions Test Completed ===")
