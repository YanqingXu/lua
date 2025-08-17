-- String operations tests
-- Tests for various string library functions

print("=== String operations tests ===")

-- Test 1: String length
print("Test 1: String length")
local str1 = "Hello"
local str2 = "Hello World"
local str3 = ""
print("  #'Hello' =", #str1)
print("  #'Hello World' =", #str2)
print("  #'' =", #str3)

-- Test 2: String concatenation
print("\nTest 2: String concatenation")
local part1 = "Hello"
local part2 = " "
local part3 = "World"
local combined = part1 .. part2 .. part3
print("  'Hello' .. ' ' .. 'World' =", combined)

-- Test 3: String comparison
print("\nTest 3: String comparison")
local a = "apple"
local b = "banana"
local c = "apple"
print("  'apple' == 'banana':", a == b)
print("  'apple' == 'apple':", a == c)
print("  'apple' < 'banana':", a < b)
print("  'banana' > 'apple':", b > a)

-- Test 4: string.len function
print("\nTest 4: string.len function")
if string and string.len then
    print("  string.len('Hello') =", string.len("Hello"))
    print("  string.len('') =", string.len(""))
else
    print("  string.len function not available")
end

-- Test 5: string.upper and string.lower
print("\nTest 5: string.upper and string.lower")
if string and string.upper and string.lower then
    local text = "Hello World"
    print("  Original:", text)
    print("  string.upper(text) =", string.upper(text))
    print("  string.lower(text) =", string.lower(text))
else
    print("  string.upper/lower functions not available")
end

-- Test 6: string.sub
print("\nTest 6: string.sub")
if string and string.sub then
    local text = "Hello World"
    print("  Original:", text)
    print("  string.sub(text, 1, 5) =", string.sub(text, 1, 5))
    print("  string.sub(text, 7) =", string.sub(text, 7))
    print("  string.sub(text, -5) =", string.sub(text, -5))
else
    print("  string.sub function not available")
end

-- Test 7: Automatic conversion between strings and numbers
print("\nTest 7: Automatic conversion between strings and numbers")
local numStr = "123"
local strNum = 456
print("  '123' + 1 =", numStr + 1)
print("  456 .. ' test' =", strNum .. " test")

-- Test 8: Multiline strings
print("\nTest 8: Multiline strings")
local multiline = [[This is a
multiline string
test]]
print("  Multiline string:")
print(multiline)

-- Test 9: Escape characters
print("\nTest 9: Escape characters")
local escaped = "First line\nSecond line\tTab character\\\"Backslash\\\""
print("  Escape character test:")
print(escaped)

-- Test 10: String repetition
print("\nTest 10: String repetition")
if string and string.rep then
    print("  string.rep('*', 5) =", string.rep("*", 5))
    print("  string.rep('abc', 3) =", string.rep("abc", 3))
else
    print("  string.rep function not available")
end

-- Test 11: String search
print("\nTest 11: String search")
if string and string.find then
    local text = "Hello World Hello"
    print("  Original:", text)
    local pos = string.find(text, "World")
    print("  string.find(text, 'World') =", pos)
    local pos2 = string.find(text, "Hello", 2)
    print("  string.find(text, 'Hello', 2) =", pos2)
else
    print("  string.find function not available")
end

-- Test 12: String formatting
print("\nTest 12: String formatting")
if string and string.format then
    local name = "Zhang San"
    local age = 25
    local formatted = string.format("Name: %s, Age: %d", name, age)
    print("  Formatted result:", formatted)
else
    print("  string.format function not available")
end

-- Test 13: String reversal
print("\nTest 13: String reversal")
if string and string.reverse then
    local text = "Hello"
    print("  string.reverse('Hello') =", string.reverse(text))
else
    print("  string.reverse function not available")
end

print("\n=== String operations tests completed ===")
