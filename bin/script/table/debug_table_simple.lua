-- 表字面量详细调试测试
print("=== Table Literal Detailed Debug Test ===")

-- 测试1: 最简单的键值对
print("Test 1: Single key-value pair")
local t1 = {x = 10}
print("t1 created")
print("t1.x =", tostring(t1.x))

-- 测试2: 对比动态赋值
print("\nTest 2: Dynamic assignment comparison")
local t2 = {}
t2.x = 10
print("t2.x =", tostring(t2.x))

print("\n=== Debug test completed ===")
