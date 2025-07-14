-- 简单的__call调试测试
print("=== Simple Call Debug Test ===")

-- 测试1: 最基本的__call
print("\nTest 1: Basic __call")
local obj = {}
setmetatable(obj, {
    __call = function(self, x)
        print("__call invoked!")
        print("self type:", type(self))
        print("x type:", type(x))
        print("x value:", x)
        return "result_" .. tostring(x)
    end
})

print("About to call obj(42)...")
local result = obj(42)
print("Call completed. Result:", result)

-- 测试2: 多返回值
print("\nTest 2: Multi-return values")
local obj2 = {}
setmetatable(obj2, {
    __call = function(self, x)
        print("Multi-return __call invoked with x =", x)
        return x, x + 1, x + 2
    end
})

print("About to call obj2(10)...")
local a, b, c = obj2(10)
print("Results: a =", a, "b =", b, "c =", c)

print("\n=== Test completed ===")
