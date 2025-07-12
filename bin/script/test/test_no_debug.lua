-- Test to verify no debug output is shown
print("=== Testing No Debug Output ===")

-- Test basic operations
local x = 5
local y = 3
print("x =", x)
print("y =", y)
print("x + y =", x + y)

-- Test table operations
local t = {a = 1, b = 2}
print("t.a =", t.a)
t.c = 3
print("t.c =", t.c)

-- Test function calls
local function add(a, b)
    return a + b
end

print("add(10, 20) =", add(10, 20))

-- Test string operations
local str = "Hello"
print("String:", str)

-- Test math operations
print("Math operations:")
print("5 * 3 =", 5 * 3)
print("10 / 2 =", 10 / 2)
print("7 % 3 =", 7 % 3)

print("=== Test Complete ===")
