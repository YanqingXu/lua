-- 逐步测试综合用例 - 第2步
print("=== Debug Comprehensive Step 2 ===")

-- 测试1: 基础功能
print("Test 1: Basic __call functionality")
local basic = {}
setmetatable(basic, {
    __call = function(self, msg)
        return "Hello, " .. tostring(msg)
    end
})
local result1 = basic("World")
print("✅ Basic call: " .. tostring(result1))

-- 测试2: 多参数和数学运算
print("\nTest 2: Multi-argument arithmetic")
local calculator = {}
setmetatable(calculator, {
    __call = function(self, op, a, b)
        if op == "add" then return a + b
        elseif op == "mul" then return a * b
        elseif op == "sub" then return a - b
        else return 0 end
    end
})
local add_result = calculator("add", 15, 25)
local mul_result = calculator("mul", 6, 7)
print("✅ Calculator add: " .. tostring(add_result))
print("✅ Calculator mul: " .. tostring(mul_result))

print("Step 2 completed")
