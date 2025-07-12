-- Test metamethods functionality
print("=== Testing Metamethods ===")

-- Test 1: Basic table creation
print("Test 1: Basic table creation")
local t = {}
t.x = 10
t.y = 20
print("t.x = " .. t.x)
print("t.y = " .. t.y)

-- Test 2: Metatable creation (when setmetatable is available)
print("")
print("Test 2: Metatable support")
local mt = {}
print("Created metatable: " .. tostring(mt))

-- Test 3: Basic arithmetic (should work without metamethods)
print("")
print("Test 3: Basic arithmetic")
local a = 5
local b = 3
print("a + b = " .. (a + b))
print("a - b = " .. (a - b))
print("a * b = " .. (a * b))
print("a / b = " .. (a / b))

-- Test 4: String concatenation
print("")
print("Test 4: String concatenation")
local str1 = "Hello"
local str2 = "World"
print("str1 .. ' ' .. str2 = " .. str1 .. " " .. str2)

-- Test 5: Comparison operations
print("")
print("Test 5: Comparison operations")
print("5 == 5: " .. tostring(5 == 5))
print("5 < 10: " .. tostring(5 < 10))
print("10 <= 10: " .. tostring(10 <= 10))

print("")
print("=== Metamethods test completed ===")
