-- 简单的 __call 元方法测试
print("=== Simple Call Test ===")

-- 测试1: 创建带 __call 的对象
print("Test 1: Creating object with __call")
local obj = {}
local meta = {
    __call = function(self, arg)
        print("__call triggered with arg: " .. tostring(arg))
        return "called_" .. tostring(arg)
    end
}

setmetatable(obj, meta)
print("Object created with __call metamethod")

-- 测试2: 检查元方法是否存在
print("\nTest 2: Checking metamethod existence")
local mt = getmetatable(obj)
print("Metatable exists: " .. tostring(mt ~= nil))
if mt then
    print("__call exists: " .. tostring(mt.__call ~= nil))
    print("__call type: " .. type(mt.__call))
end

-- 测试3: 尝试最简单的调用
print("\nTest 3: Attempting simple call")
print("About to call obj('test')...")

-- 这里可能会出问题
local result = obj("test")
print("Call successful! Result: " .. tostring(result))

print("\n=== Simple call test completed ===")
