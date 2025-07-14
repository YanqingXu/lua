-- 测试多返回值
print("=== Multi-Return Value Test ===")

local obj = {}
setmetatable(obj, {
    __call = function(self, x)
        print("__call with x =", x)
        return x, x + 1, x + 2
    end
})

print("Calling obj(10) with multiple return values...")
local a, b, c = obj(10)
print("Results: a =", a, "b =", b, "c =", c)
print("Expected: a = 10, b = 11, c = 12")
