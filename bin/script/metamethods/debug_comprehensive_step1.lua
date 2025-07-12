-- 逐步测试综合用例 - 第1步
print("=== Debug Comprehensive Step 1 ===")

-- 只测试第一个用例
print("Test 1: Basic __call functionality")
local basic = {}
setmetatable(basic, {
    __call = function(self, msg)
        return "Hello, " .. tostring(msg)
    end
})
local result1 = basic("World")
print("✅ Basic call: " .. tostring(result1))

print("Step 1 completed")
