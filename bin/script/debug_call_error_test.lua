-- 测试会产生错误的调用
print("=== Testing Error Cases ===")

-- 测试1: 调用没有 __call 的表
print("Test 1: Calling table without __call")
local obj = {}
obj()  -- 这应该产生错误
