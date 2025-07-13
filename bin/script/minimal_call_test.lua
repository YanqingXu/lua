-- 最小化的__call测试
print("开始测试")

local mt = {
    __call = function(t, x)
        return x + 1
    end
}

local obj = setmetatable({}, mt)
print("对象创建完成")

local result = obj(5)
print("结果:", result)

print("测试完成")
