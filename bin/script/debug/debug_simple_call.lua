-- 最简单的__call测试
print("=== Simple Call Debug ===")

local obj = {}
setmetatable(obj, {
    __call = function(self, x)
        print("__call invoked: self =", self, "x =", x)
        return x * 2
    end
})

print("Calling obj(5)...")
local result = obj(5)
print("Result:", result)
