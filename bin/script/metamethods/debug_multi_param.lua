-- 测试多参数调用
print("=== Debug Multi Parameter Call ===")

local obj = {}
setmetatable(obj, {
    __call = function(self, a, b, c)
        print("Called with: " .. tostring(a) .. ", " .. tostring(b) .. ", " .. tostring(c))
        return tostring(a) .. "_" .. tostring(b) .. "_" .. tostring(c)
    end
})

print("Single multi-param call:")
local result1 = obj("first", "second", "third")
print("Result: " .. tostring(result1))

print("Test completed")
