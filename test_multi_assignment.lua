-- Test multi-return value assignment syntax
print("=== Multi-Return Value Assignment Test ===")

-- Test 1: Basic multi-assignment with __call metamethod
print("\n--- Test 1: Basic multi-assignment ---")
local obj1 = {}
setmetatable(obj1, {
    __call = function(self, x)
        print("__call called with x =", x)
        return x, x * 2, x * 3
    end
})

print("Testing: local a, b, c = obj1(5)")
local a, b, c = obj1(5)
print("Results: a =", a, "b =", b, "c =", c)
print("Expected: a = 5, b = 10, c = 15")

-- Test 2: More variables than return values
print("\n--- Test 2: More variables than return values ---")
local obj2 = {}
setmetatable(obj2, {
    __call = function(self)
        print("__call returning only two values")
        return "first", "second"
    end
})

print("Testing: local x, y, z, w = obj2()")
local x, y, z, w = obj2()
print("Results: x =", x, "y =", y, "z =", z, "w =", w)
print("Expected: x = 'first', y = 'second', z = nil, w = nil")

-- Test 3: Fewer variables than return values
print("\n--- Test 3: Fewer variables than return values ---")
local obj3 = {}
setmetatable(obj3, {
    __call = function(self)
        print("__call returning many values")
        return 1, 2, 3, 4, 5
    end
})

print("Testing: local p, q = obj3()")
local p, q = obj3()
print("Results: p =", p, "q =", q)
print("Expected: p = 1, q = 2 (excess values discarded)")

-- Test 4: Single variable assignment (should still work)
print("\n--- Test 4: Single variable assignment ---")
local obj4 = {}
setmetatable(obj4, {
    __call = function(self, val)
        print("__call returning single value")
        return val * 10
    end
})

print("Testing: local single = obj4(7)")
local single = obj4(7)
print("Results: single =", single)
print("Expected: single = 70")

-- Test 5: No return values
print("\n--- Test 5: No return values ---")
local obj5 = {}
setmetatable(obj5, {
    __call = function(self)
        print("__call returning no values")
        -- return nothing
    end
})

print("Testing: local m, n = obj5()")
local m, n = obj5()
print("Results: m =", m, "n =", n)
print("Expected: m = nil, n = nil")

print("\n=== Test completed ===")
