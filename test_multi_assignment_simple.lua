-- Simple test for multi-assignment syntax
print("=== Simple Multi-Assignment Test ===")

-- Test 1: Basic multi-assignment with function call
print("\n--- Test 1: Basic multi-assignment ---")

-- Create a simple function that returns multiple values
function multiReturn(x)
    print("multiReturn called with x =", x)
    return x, x * 2, x * 3
end

print("Testing: local a, b, c = multiReturn(5)")
local a, b, c = multiReturn(5)
print("Results: a =", a, "b =", b, "c =", c)
print("Expected: a = 5, b = 10, c = 15")

-- Test 2: More variables than return values
print("\n--- Test 2: More variables than return values ---")

function twoValues()
    print("twoValues called")
    return "first", "second"
end

print("Testing: local x, y, z, w = twoValues()")
local x, y, z, w = twoValues()
print("Results: x =", x, "y =", y, "z =", z, "w =", w)
print("Expected: x = 'first', y = 'second', z = nil, w = nil")

-- Test 3: Fewer variables than return values
print("\n--- Test 3: Fewer variables than return values ---")

function manyValues()
    print("manyValues called")
    return 1, 2, 3, 4, 5
end

print("Testing: local p, q = manyValues()")
local p, q = manyValues()
print("Results: p =", p, "q =", q)
print("Expected: p = 1, q = 2 (excess values discarded)")

print("\n=== Test completed ===")
