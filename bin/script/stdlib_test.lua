-- Standard Library Test Script
-- Tests the newly fixed string and math libraries

print("=== Standard Library Test ===")

-- Test basic functions
print("Testing basic functions...")
print("type(42):", type(42))
print("type('hello'):", type("hello"))

-- Test string library functions
print("\n=== String Library Tests ===")

-- String length
local str = "hello"
print("string.len('hello'):", string.len(str))

-- String case conversion
print("string.upper('hello'):", string.upper(str))
print("string.lower('WORLD'):", string.lower("WORLD"))

-- String reverse
print("string.reverse('hello'):", string.reverse(str))

-- String repetition
print("string.rep('hi', 3):", string.rep("hi", 3))

-- Test math library functions
print("\n=== Math Library Tests ===")

-- Basic math functions
print("math.abs(-42):", math.abs(-42))
print("math.floor(3.7):", math.floor(3.7))
print("math.ceil(3.2):", math.ceil(3.2))

-- Trigonometric functions
print("math.sin(0):", math.sin(0))
print("math.cos(0):", math.cos(0))

-- Power and logarithmic functions
print("math.sqrt(16):", math.sqrt(16))
print("math.pow(2, 3):", math.pow(2, 3))

-- Min/max functions
print("math.min(5, 3, 8):", math.min(5, 3, 8))
print("math.max(5, 3, 8):", math.max(5, 3, 8))

print("\n=== Test Complete ===")
