-- 逐步增加复杂度的 __call 测试
print("=== Step by Step Call Test ===")

-- 测试1: 基础 __call（已知工作）
print("Test 1: Basic __call")
local obj1 = {}
local meta1 = {
    __call = function(self, arg)
        print("Basic __call: " .. tostring(arg))
        return "basic_" .. tostring(arg)
    end
}
setmetatable(obj1, meta1)
local result1 = obj1("test1")
print("Result1: " .. tostring(result1))

-- 测试2: 多参数 __call
print("\nTest 2: Multi-argument __call")
local obj2 = {}
local meta2 = {
    __call = function(self, a, b)
        print("Multi __call: " .. tostring(a) .. ", " .. tostring(b))
        return tostring(a) .. "_" .. tostring(b)
    end
}
setmetatable(obj2, meta2)
local result2 = obj2("hello", "world")
print("Result2: " .. tostring(result2))

-- 测试3: 无额外参数的 __call
print("\nTest 3: No-argument __call")
local obj3 = {}
local meta3 = {
    __call = function(self)
        print("No-arg __call executed")
        return "no_args"
    end
}
setmetatable(obj3, meta3)
local result3 = obj3()
print("Result3: " .. tostring(result3))

-- 测试4: 数字参数的 __call
print("\nTest 4: Numeric arguments __call")
local obj4 = {}
local meta4 = {
    __call = function(self, x, y)
        print("Numeric __call: " .. tostring(x) .. " + " .. tostring(y))
        return x + y
    end
}
setmetatable(obj4, meta4)
local result4 = obj4(10, 20)
print("Result4: " .. tostring(result4))

print("\n=== All step-by-step tests completed ===")
