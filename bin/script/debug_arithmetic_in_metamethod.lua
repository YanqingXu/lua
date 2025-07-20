-- Debug arithmetic operations in metamethod context
print("=== Arithmetic in Metamethod Debug ===")

print("Testing basic arithmetic outside metamethod...")
local a = 5
local b = 3
local c = 100
print("a =", a)
print("b =", b)
print("c =", c)
print("a + b =", a + b)
print("a + b + c =", a + b + c)

print("Testing arithmetic inside regular function...")
local function test_arithmetic(x, y)
    print("Inside function: x =", x, ", y =", y)
    local sum1 = x + y
    print("x + y =", sum1)
    local sum2 = sum1 + 100
    print("(x + y) + 100 =", sum2)
    return sum2
end

local result1 = test_arithmetic(5, 3)
print("Function result:", result1)

print("Testing arithmetic inside metamethod...")
local obj = {}
local meta = {
    __call = function(self, x, y)
        print("Inside metamethod: x =", x, ", y =", y)
        local sum1 = x + y
        print("x + y =", sum1)
        local sum2 = sum1 + 100
        print("(x + y) + 100 =", sum2)
        return sum2
    end
}
setmetatable(obj, meta)

local result2 = obj(5, 3)
print("Metamethod result:", result2)

print("=== Arithmetic in Metamethod Debug Complete ===")
