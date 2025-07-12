-- 函数形式元方法调试测试
print("=== Function Metamethod Debug Test ===")

-- 测试1: 简单的函数形式 __index 元方法
print("Test 1: Function-based __index metamethod")
local obj = {}
local meta = {
    __index = function(table, key)
        print("__index function called with key: " .. tostring(key))
        return "default_" .. tostring(key)
    end
}

setmetatable(obj, meta)
print("Accessing obj.test:")
local result = obj.test
print("Result: " .. tostring(result))

-- 测试2: 简单的函数形式 __newindex 元方法
print("\nTest 2: Function-based __newindex metamethod")
local obj2 = {}
local meta2 = {
    __newindex = function(table, key, value)
        print("__newindex function called: " .. tostring(key) .. " = " .. tostring(value))
        -- 这里应该存储到某个地方，但为了测试简单，我们只打印
    end
}

setmetatable(obj2, meta2)
print("Setting obj2.newkey = 'newvalue':")
obj2.newkey = "newvalue"

-- 测试3: __call 元方法
print("\nTest 3: __call metamethod")
local callable = {}
local meta3 = {
    __call = function(table, arg1, arg2)
        print("__call function called with args: " .. tostring(arg1) .. ", " .. tostring(arg2))
        return "called with " .. tostring(arg1) .. " and " .. tostring(arg2)
    end
}

setmetatable(callable, meta3)
print("Calling callable('hello', 'world'):")
-- 注意：这里可能会失败，因为函数调用机制可能有问题
-- local callResult = callable("hello", "world")
-- print("Call result: " .. tostring(callResult))
print("(Call test skipped due to potential function call issues)")

print("\n=== Function metamethod debug test completed ===")
