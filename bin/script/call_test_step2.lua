-- 测试实际的函数调用
print("开始__call调用测试")

local mt = {
    __call = function(t)
        print("__call被调用了")
        return "success"
    end
}

local obj = setmetatable({}, mt)
print("对象创建完成")

-- 检查元表
local metatable = getmetatable(obj)
print("元表存在:", metatable ~= nil)
if metatable then
    print("__call存在:", metatable.__call ~= nil)
    print("__call类型:", type(metatable.__call))
end

print("准备调用obj()")

-- 尝试调用
obj()

print("调用完成")
