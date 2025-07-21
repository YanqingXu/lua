-- Base Library Function Tests
-- Testing core functions: print(), type(), tostring(), tonumber(), etc.
-- Based on current development plan status

print("=== Base Library Function Tests ===")
print()

-- Test 1: print() function (已知工作正常)
print("Test 1: print() function")
print("Single argument:", "Hello World")
print("Multiple arguments:", 42, true, nil, "test")
print("Numbers:", 123, 3.14, -456)
print("Booleans:", true, false)
print()

-- Test 2: type() function (已知工作正常)
print("Test 2: type() function")
print("type(42):", type(42))
print("type('hello'):", type("hello"))
print("type(true):", type(true))
print("type(false):", type(false))
print("type(nil):", type(nil))
print("type({}):", type({}))
print("type(print):", type(print))
print()

-- Test 3: tostring() function (已知有问题 - 返回错误字符串)
print("Test 3: tostring() function")
print("tostring(42):", tostring(42))
print("tostring(3.14):", tostring(3.14))
print("tostring(true):", tostring(true))
print("tostring(false):", tostring(false))
print("tostring(nil):", tostring(nil))
print("tostring('hello'):", tostring("hello"))
print()

-- Test 4: tonumber() function (已知有问题 - 总是返回nil)
print("Test 4: tonumber() function")
print("tonumber('42'):", tonumber("42"))
print("tonumber('3.14'):", tonumber("3.14"))
print("tonumber('123.456'):", tonumber("123.456"))
print("tonumber('invalid'):", tonumber("invalid"))
print("tonumber('0xFF', 16):", tonumber("0xFF", 16))
print("tonumber('1010', 2):", tonumber("1010", 2))
print()

-- Test 5: Basic error handling functions
print("Test 5: Error handling functions")

-- Test assert() function
print("Testing assert():")
local result1 = assert(true, "This should pass")
print("assert(true) result:", result1)

local result2 = assert(42, "This should return 42")
print("assert(42) result:", result2)

local result3 = assert("hello", "This should return 'hello'")
print("assert('hello') result:", result3)

-- Note: assert(false) would terminate the program, so we skip it
print("Skipping assert(false) test to avoid program termination")
print()

-- Test 6: Variable assignment and basic operations
print("Test 6: Variable assignment and operations")
local num = 42
local str = "test"
local bool = true
local tbl = {1, 2, 3}

print("num =", num, "type:", type(num))
print("str =", str, "type:", type(str))
print("bool =", bool, "type:", type(bool))
print("tbl =", tbl, "type:", type(tbl))
print()

-- Test 7: String concatenation (测试字符串连接操作)
print("Test 7: String concatenation")
local str1 = "Hello"
local str2 = "World"
local result = str1 .. " " .. str2
print("'" .. str1 .. "' .. ' ' .. '" .. str2 .. "' =", result)

local num_str = "Number: " .. tostring(42)
print("'Number: ' .. tostring(42) =", num_str)
print()

-- Test 8: Basic arithmetic operations
print("Test 8: Basic arithmetic operations")
local a = 10
local b = 3
print("a =", a, ", b =", b)
print("a + b =", a + b)
print("a - b =", a - b)
print("a * b =", a * b)
print("a / b =", a / b)
print("a % b =", a % b)
print()

-- Test 9: Comparison operations
print("Test 9: Comparison operations")
print("10 == 10:", 10 == 10)
print("10 ~= 5:", 10 ~= 5)
print("10 > 5:", 10 > 5)
print("10 < 5:", 10 < 5)
print("10 >= 10:", 10 >= 10)
print("10 <= 5:", 10 <= 5)
print()

-- Test 10: Logical operations
print("Test 10: Logical operations")
print("true and true:", true and true)
print("true and false:", true and false)
print("true or false:", true or false)
print("false or false:", false or false)
print("not true:", not true)
print("not false:", not false)
print()

print("=== Base Library Tests Complete ===")
print("Expected issues:")
print("- tostring() may return incorrect strings")
print("- tonumber() may always return nil")
print("- Other functions should work correctly")
