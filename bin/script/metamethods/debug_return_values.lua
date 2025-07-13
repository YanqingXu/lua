-- Debug return value handling
print("=== Debug Return Value Handling ===")

-- Test 1: Regular function with multiple returns
print("\n--- Test 1: Regular function ---")
function regular_multi(x)
    print("Regular function returning:", x, x*2, x*3)
    return x, x*2, x*3
end

local a, b, c = regular_multi(5)
print("Regular function results:")
print("  a =", a)
print("  b =", b)
print("  c =", c)

-- Test 2: __call metamethod with multiple returns
print("\n--- Test 2: __call metamethod ---")
local call_obj = {}
setmetatable(call_obj, {
    __call = function(self, x)
        print("__call metamethod returning:", x, x*2, x*3)
        return x, x*2, x*3
    end
})

local d, e, f = call_obj(5)
print("__call metamethod results:")
print("  d =", d)
print("  e =", e)
print("  f =", f)

-- Test 3: Single return value
print("\n--- Test 3: Single return value ---")
local single_obj = {}
setmetatable(single_obj, {
    __call = function(self, x)
        print("Single return:", x*10)
        return x*10
    end
})

local g = single_obj(5)
print("Single return result:")
print("  g =", g)

print("\n=== Debug completed ===")
