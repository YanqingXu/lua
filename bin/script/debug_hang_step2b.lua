-- 测试select函数调用
print("开始测试select调用")

print("测试1: select('#', 1, 2, 3)")
local result = select("#", 1, 2, 3)
print("结果:", result)

print("测试完成")
