-- 测试边界情况和复杂场景
print("=== 边界情况测试 ===")

-- 测试1: 复杂的比较链
print("\n1. 测试复杂比较链:")
local a, b, c = 1, 2, 3
if a < b and b < c then
    print("  ✓ 比较链 a < b < c 正确")
else
    print("  ✗ 比较链 a < b < c 失败")
end

-- 测试2: 混合类型比较的错误处理
print("\n2. 测试混合类型比较:")
local num = 5
local str = "hello"
-- 这应该不会崩溃，但可能返回false
if num == str then
    print("  ✗ 数字和字符串错误地相等")
else
    print("  ✓ 数字和字符串正确地不相等")
end

-- 测试3: nil值比较
print("\n3. 测试nil值比较:")
local x = nil
local y = nil
if x == y then
    print("  ✓ nil == nil 正确")
else
    print("  ✗ nil == nil 失败")
end

if x == 0 then
    print("  ✗ nil == 0 错误地为真")
else
    print("  ✓ nil == 0 正确为假")
end

-- 测试4: 复杂字符串连接
print("\n4. 测试复杂字符串连接:")
local s1, s2, s3, s4 = "A", "B", "C", "D"
local result = s1 .. s2 .. s3 .. s4
print("  四值连接: " .. result .. " (期望: ABCD)")

-- 测试5: 数字和字符串的连接
print("\n5. 测试数字字符串连接:")
local n1, n2 = 123, 456
local mixed = "Number: " .. n1 .. " and " .. n2
print("  混合连接: " .. mixed)

-- 测试6: 嵌套条件和循环
print("\n6. 测试嵌套条件和循环:")
local count = 0
for i = 1, 5 do
    for j = 1, 3 do
        if i <= 3 and j <= 2 then
            count = count + 1
        end
    end
end
print("  嵌套循环计数: " .. count .. " (期望: 6)")

-- 测试7: 复杂的逻辑表达式
print("\n7. 测试复杂逻辑表达式:")
local x, y, z = 10, 20, 30
if (x < y) and (y < z) and (x + y < z + 10) then
    print("  ✓ 复杂逻辑表达式正确")
else
    print("  ✗ 复杂逻辑表达式失败")
end

-- 测试8: 条件运算符的短路求值
print("\n8. 测试条件短路:")
local flag = true
if flag and (1 == 1) then
    print("  ✓ AND短路正确")
else
    print("  ✗ AND短路失败")
end

if flag or (1 == 2) then
    print("  ✓ OR短路正确")
else
    print("  ✗ OR短路失败")
end

print("\n=== 边界测试完成 ===")
