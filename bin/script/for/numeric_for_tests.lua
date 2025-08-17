-- 数值for循环测试
-- 测试基本的数值for循环功能

print("=== 数值for循环测试 ===")

-- 测试1: 基本递增循环
print("测试1: 基本递增循环 (1到5)")
for i = 1, 5 do
    print("  i =", i)
end

-- 测试2: 基本递减循环
print("\n测试2: 基本递减循环 (5到1)")
for i = 5, 1, -1 do
    print("  i =", i)
end

-- 测试3: 步长为2的循环
print("\n测试3: 步长为2的循环 (0到10)")
for i = 0, 10, 2 do
    print("  i =", i)
end

-- 测试4: 负步长循环
print("\n测试4: 负步长循环 (10到0，步长-2)")
for i = 10, 0, -2 do
    print("  i =", i)
end

-- 测试5: 单次循环
print("\n测试5: 单次循环 (5到5)")
for i = 5, 5 do
    print("  i =", i)
end

-- 测试6: 空循环（不应执行）
print("\n测试6: 空循环测试 (1到0)")
local executed = false
for i = 1, 0 do
    executed = true
    print("  这不应该被打印")
end
if not executed then
    print("✓ 空循环正确地没有执行")
else
    print("✗ 空循环错误地执行了")
end

-- 测试7: 浮点数循环
print("\n测试7: 浮点数循环 (0.5到2.5，步长0.5)")
for i = 0.5, 2.5, 0.5 do
    print("  i =", i)
end

-- 测试8: 嵌套for循环
print("\n测试8: 嵌套for循环 (2x3矩阵)")
for i = 1, 2 do
    for j = 1, 3 do
        print("  (" .. i .. "," .. j .. ")")
    end
end

-- 测试9: 循环中的break
print("\n测试9: 循环中的break (1到10，在5处break)")
for i = 1, 10 do
    if i == 5 then
        print("  在i=5处break")
        break
    end
    print("  i =", i)
end

-- 测试10: 循环变量作用域
print("\n测试10: 循环变量作用域测试")
local outerI = 100
for i = 1, 3 do
    print("  循环内 i =", i)
end
print("  循环外 outerI =", outerI)
-- 注意：循环变量i在循环外不可访问

-- 测试11: 大范围循环（测试性能）
print("\n测试11: 大范围循环测试 (计算1到1000的和)")
local sum = 0
for i = 1, 1000 do
    sum = sum + i
end
print("  1到1000的和 =", sum)
print("  期望值 = 500500")
if sum == 500500 then
    print("✓ 大范围循环计算正确")
else
    print("✗ 大范围循环计算错误")
end

print("\n=== 数值for循环测试完成 ===")
