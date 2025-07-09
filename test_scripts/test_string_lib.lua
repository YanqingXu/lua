-- String Library Function Tests
-- Testing string manipulation functions

print("=== String Library Tests ===")

-- Test 1: Basic string functions
print("Test 1: Basic string functions")
local str = "Hello, World!"
print("string.len('" .. str .. "') =", string.len(str))
print("string.upper('" .. str .. "') =", string.upper(str))
print("string.lower('" .. str .. "') =", string.lower(str))
print("string.reverse('" .. str .. "') =", string.reverse(str))

-- Test 2: String substring
print("\nTest 2: String substring")
print("string.sub('Hello', 1, 3) =", string.sub("Hello", 1, 3))
print("string.sub('Hello', 2) =", string.sub("Hello", 2))
print("string.sub('Hello', -3) =", string.sub("Hello", -3))
print("string.sub('Hello', 2, -2) =", string.sub("Hello", 2, -2))

-- Test 3: String repetition
print("\nTest 3: String repetition")
print("string.rep('Ha', 3) =", string.rep("Ha", 3))
print("string.rep('*', 5) =", string.rep("*", 5))
print("string.rep('A', 0) =", string.rep("A", 0))

-- Test 4: String length with different inputs
print("\nTest 4: String length tests")
print("string.len('') =", string.len(""))
print("string.len('a') =", string.len("a"))
print("string.len('Hello World') =", string.len("Hello World"))
print("string.len('中文测试') =", string.len("中文测试"))

-- Test 5: Case conversion edge cases
print("\nTest 5: Case conversion edge cases")
print("string.upper('hello123') =", string.upper("hello123"))
print("string.lower('WORLD456') =", string.lower("WORLD456"))
print("string.upper('') =", string.upper(""))
print("string.lower('') =", string.lower(""))

-- Test 6: Complex string operations
print("\nTest 6: Complex string operations")
local text = "Lua Programming"
print("Original: '" .. text .. "'")
print("Length:", string.len(text))
print("Upper:", string.upper(text))
print("Lower:", string.lower(text))
print("First 3 chars:", string.sub(text, 1, 3))
print("Last 3 chars:", string.sub(text, -3))
print("Reversed:", string.reverse(text))

-- Test 7: String functions with numbers
print("\nTest 7: String functions with numbers")
print("string.len(tostring(12345)) =", string.len(tostring(12345)))
print("string.upper(tostring(true)) =", string.upper(tostring(true)))

print("\n=== String Library Tests Complete ===")
