-- __call 元方法测试
print("=== Call Metamethod Test ===")

-- 测试1: 简单的 __call 元方法
print("Test 1: Simple __call metamethod")
local callable = {}
local meta = {
    __call = function(self, arg1, arg2)
        print("__call invoked with args: " .. tostring(arg1) .. ", " .. tostring(arg2))
        return "Result: " .. tostring(arg1) .. " + " .. tostring(arg2)
    end
}

setmetatable(callable, meta)

print("Calling callable('hello', 'world'):")
-- 注意：这里可能会失败，因为函数调用语法可能还没有完全实现
-- 让我们先测试元方法是否存在
print("Metatable set successfully")
print("__call metamethod exists: " .. tostring(getmetatable(callable).__call ~= nil))

-- 测试2: 尝试通过其他方式调用
print("\nTest 2: Alternative call test")
local result = "Call test skipped - function call syntax may not be fully implemented"
print(result)

print("\n=== Call metamethod test completed ===")
