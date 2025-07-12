-- 测试可能导致 "Attempt to call a non-callable value" 错误的情况
print("=== Debug Call Error ===")

-- 情况1: 尝试调用没有元方法的表
print("Test 1: Calling table without __call")
local obj1 = {}
-- 这应该会产生错误
-- obj1()

-- 情况2: 尝试调用数字
print("Test 2: Calling number")
local num = 42
-- 这应该会产生错误
-- num()

-- 情况3: 尝试调用字符串
print("Test 3: Calling string")
local str = "hello"
-- 这应该会产生错误
-- str()

-- 情况4: __call 不是函数
print("Test 4: __call is not a function")
local obj4 = {}
setmetatable(obj4, {
    __call = "not a function"
})
-- 这应该会产生错误
-- obj4()

-- 情况5: 正常的 __call（应该工作）
print("Test 5: Normal __call (should work)")
local obj5 = {}
setmetatable(obj5, {
    __call = function(self)
        return "success"
    end
})
local result = obj5()
print("Result: " .. tostring(result))

print("Test completed")
