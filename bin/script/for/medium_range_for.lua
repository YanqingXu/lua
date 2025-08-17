-- 中等范围for循环测试
print("开始中等范围for循环测试")
local sum = 0
for i = 1, 10 do
    sum = sum + i
end
print("1到10的和 =", sum)
print("期望值 = 55")

-- 测试更大范围
sum = 0
for i = 1, 100 do
    sum = sum + i
end
print("1到100的和 =", sum)
print("期望值 = 5050")
