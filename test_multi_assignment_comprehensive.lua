-- Comprehensive test for multi-assignment syntax
print("=== Comprehensive Multi-Assignment Test ===")

-- Test 1: Single variable assignment (should use traditional LocalStmt)
print("\n--- Test 1: Single variable assignment ---")
local single = 42
print("single =", single)

-- Test 2: Multiple variables with single value (should use MultiLocalStmt)
print("\n--- Test 2: Multiple variables with single value ---")
local a, b, c = 100
print("a =", a, "b =", b, "c =", c)
print("Expected: a = 100, b = nil, c = nil")

-- Test 3: Multiple variables with multiple values
print("\n--- Test 3: Multiple variables with multiple values ---")
local x, y, z = 1, 2, 3
print("x =", x, "y =", y, "z =", z)
print("Expected: x = 1, y = 2, z = 3")

-- Test 4: More variables than values
print("\n--- Test 4: More variables than values ---")
local p, q, r, s = "first", "second"
print("p =", p, "q =", q, "r =", r, "s =", s)
print("Expected: p = 'first', q = 'second', r = nil, s = nil")

-- Test 5: Fewer variables than values
print("\n--- Test 5: Fewer variables than values ---")
local m, n = "alpha", "beta", "gamma", "delta"
print("m =", m, "n =", n)
print("Expected: m = 'alpha', n = 'beta' (excess values discarded)")

-- Test 6: No initializers
print("\n--- Test 6: No initializers ---")
local u, v, w
print("u =", u, "v =", v, "w =", w)
print("Expected: u = nil, v = nil, w = nil")

-- Test 7: Function call with multi-assignment (this is the key test)
print("\n--- Test 7: Function call with multi-assignment ---")

-- Create a simple function that should return multiple values
function testFunc()
    return "result1", "result2", "result3"
end

print("Testing: local f1, f2, f3 = testFunc()")
local f1, f2, f3 = testFunc()
print("f1 =", f1, "f2 =", f2, "f3 =", f3)
print("Expected: f1 = 'result1', f2 = 'result2', f3 = 'result3'")

-- Test 8: Mixed scenarios
print("\n--- Test 8: Mixed scenarios ---")
local mix1, mix2 = 999
print("mix1 =", mix1, "mix2 =", mix2)
print("Expected: mix1 = 999, mix2 = nil")

print("\n=== Test completed ===")
