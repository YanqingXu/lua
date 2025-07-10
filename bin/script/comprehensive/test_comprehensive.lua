-- Comprehensive Standard Library Tests
-- Testing integration and complex scenarios

print("=== Comprehensive Standard Library Tests ===")

-- Test 1: Mixed function usage
print("Test 1: Mixed function usage")
local num = 42
local str = "Hello"
print("Number:", num, "Type:", type(num))
print("String:", str, "Type:", type(str), "Length:", string.len(str))
print("Number as string:", tostring(num), "Length:", string.len(tostring(num)))

-- Test 2: Mathematical calculations with output
print("\nTest 2: Mathematical calculations")
local a = 3.14159
local b = 2.71828
print("a =", a, "b =", b)
print("a + b =", a + b)
print("sqrt(a^2 + b^2) =", math.sqrt(math.pow(a, 2) + math.pow(b, 2)))
print("sin(a) =", math.sin(a))
print("cos(b) =", math.cos(b))

-- Test 3: String manipulation chain
print("\nTest 3: String manipulation chain")
local original = "Lua Programming Language"
print("Original:", original)
local upper = string.upper(original)
print("Upper:", upper)
local reversed = string.reverse(upper)
print("Reversed:", reversed)
local substring = string.sub(reversed, 1, 10)
print("First 10 chars of reversed:", substring)

-- Test 4: Type checking and conversion
print("\nTest 4: Type checking and conversion")
local values = {42, "hello", true, nil, 3.14}
for i = 1, 5 do
    local val = values[i]
    print("Value:", val, "Type:", type(val), "String:", tostring(val))
end

-- Test 5: Mathematical constants and functions
print("\nTest 5: Mathematical constants and functions")
print("pi =", math.pi)
print("huge =", math.huge)
print("sin(pi/2) =", math.sin(math.pi/2))
print("cos(pi) =", math.cos(math.pi))
print("log(e) =", math.log(math.exp(1)))

-- Test 6: Random number generation
print("\nTest 6: Random number generation")
math.randomseed(42)
print("Random numbers:")
for i = 1, 5 do
    print("  ", i, ":", math.random())
end

-- Test 7: String and number interaction
print("\nTest 7: String and number interaction")
local numStr = "123.45"
local num = tonumber(numStr)
print("String:", numStr, "Type:", type(numStr))
print("Converted:", num, "Type:", type(num))
if num then
    print("Math operations on converted number:")
    print("  sqrt:", math.sqrt(num))
    print("  floor:", math.floor(num))
    print("  ceil:", math.ceil(num))
end

-- Test 8: Complex string operations
print("\nTest 8: Complex string operations")
local text = "The quick brown fox jumps over the lazy dog"
print("Original text:", text)
print("Length:", string.len(text))
print("First word:", string.sub(text, 1, 3))
print("Last word:", string.sub(text, -3))
print("Middle part:", string.sub(text, 10, 20))

print("\n=== Comprehensive Tests Complete ===")
