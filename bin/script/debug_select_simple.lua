-- 简单测试select的字符串比较
print("开始测试")

print("测试1: 直接调用select")
local result1 = select("#", "a", "b")
print("结果1:", result1)

print("测试完成")
