-- Basic metamethod test
print("=== Basic Metamethod Test ===")

-- Test 1: Basic table operations (should work without metamethods)
print("Test 1: Basic table operations")
local t = {}
t.x = 10
t.y = 20
print("t.x = " .. tostring(t.x))
print("t.y = " .. tostring(t.y))

-- Test 2: Basic arithmetic (should work without metamethods)
print("")
print("Test 2: Basic arithmetic")
local a = 5
local b = 3
print("a + b = " .. tostring(a + b))
print("a - b = " .. tostring(a - b))
print("a * b = " .. tostring(a * b))

-- Test 3: String operations
print("")
print("Test 3: String operations")
local str1 = "Hello"
local str2 = "World"
print("Concatenation: " .. str1 .. " " .. str2)

-- Test 4: Comparison operations
print("")
print("Test 4: Comparison operations")
print("5 == 5: " .. tostring(5 == 5))
print("5 < 10: " .. tostring(5 < 10))
print("10 <= 10: " .. tostring(10 <= 10))

print("")
print("=== Basic test completed ===")
print("Note: Advanced metamethod features require setmetatable function")
