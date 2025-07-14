-- 简单的寄存器测试
print("=== Register Assignment Test ===")

local obj = {}
setmetatable(obj, {
    __call = function(self, x)
        return x, x + 1, x + 2
    end
})

-- 测试多返回值赋值
local a, b, c = obj(100)
print("a =", a)
print("b =", b) 
print("c =", c)
