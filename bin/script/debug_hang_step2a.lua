-- 测试select函数
print("开始测试select")

print("测试1: 直接调用select")
-- local result = select("#", 1, 2, 3)
-- print("select结果:", result)

print("测试2: 检查select是否存在")
print("select函数:", select)
print("select类型:", type(select))

print("测试完成")
