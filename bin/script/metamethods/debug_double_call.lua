-- 测试同一对象的两次调用
print("=== Debug Double Call ===")

local obj = {}
setmetatable(obj, {
    __call = function(self, msg)
        print("Called with: " .. tostring(msg))
        return "result_" .. tostring(msg)
    end
})

print("First call:")
local result1 = obj("first")
print("First result: " .. tostring(result1))

print("Second call:")
local result2 = obj("second")
print("Second result: " .. tostring(result2))

print("Test completed")
