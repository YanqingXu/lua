-- 测试寄存器分配
print("=== Register Assignment Debug ===")

local obj = {}
setmetatable(obj, {
    __call = function(self, x)
        return x, x + 1, x + 2
    end
})

-- 单独测试每个变量的赋值
print("Testing individual assignments:")

local a = obj(100)
print("a =", a)

local b = obj(200) 
print("b =", b)

local c = obj(300)
print("c =", c)
