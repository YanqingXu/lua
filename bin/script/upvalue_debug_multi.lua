-- Debug multiple upvalues
print("=== Multiple Upvalue Debug ===")

local a = 10
local b = 20

function test()
    print("a =", a)
    print("b =", b)
    return a, b
end

print("Calling test function...")
local ra, rb = test()
print("Results: a =", ra, "b =", rb)

print("=== Debug completed ===")
