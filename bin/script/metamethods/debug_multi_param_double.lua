-- 测试多参数的双重调用
print("=== Debug Multi Parameter Double Call ===")

local obj = {}
setmetatable(obj, {
    __call = function(self, a, b, c)
        print("Called with: " .. tostring(a) .. ", " .. tostring(b) .. ", " .. tostring(c))
        return tostring(a) .. "_" .. tostring(b) .. "_" .. tostring(c)
    end
})

print("First multi-param call:")
local result1 = obj("first", "second", "third")
print("First result: " .. tostring(result1))

print("Second multi-param call:")
local result2 = obj("fourth", "fifth", "sixth")
print("Second result: " .. tostring(result2))

print("Test completed")
