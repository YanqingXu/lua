-- 简单的__call测试
print("开始测试")

-- 最基本的测试
local mt = {}
print("创建元表")

mt.__call = function(t)
    print("__call被调用")
    return "result"
end
print("设置__call元方法")

local obj = {}
print("创建对象")

setmetatable(obj, mt)
print("设置元表完成")

print("测试完成")
