-- 基础if语句测试
-- 测试基本的if-then-else结构

print("=== 基础if语句测试 ===")

-- 测试1: 简单的if语句
print("测试1: 简单if语句")
local x = 5
if x > 3 then
    print("✓ x大于3")
else
    print("✗ x不大于3")
end

-- 测试2: if-else语句
print("\n测试2: if-else语句")
local y = 2
if y > 5 then
    print("✗ y大于5")
else
    print("✓ y不大于5")
end

-- 测试3: if-elseif-else语句
print("\n测试3: if-elseif-else语句")
local score = 85
if score >= 90 then
    print("✗ 成绩为A")
elseif score >= 80 then
    print("✓ 成绩为B")
elseif score >= 70 then
    print("✗ 成绩为C")
else
    print("✗ 成绩为D")
end

-- 测试4: 布尔值测试
print("\n测试4: 布尔值测试")
local flag = true
if flag then
    print("✓ flag为真")
else
    print("✗ flag为假")
end

local flag2 = false
if flag2 then
    print("✗ flag2为真")
else
    print("✓ flag2为假")
end

-- 测试5: nil值测试
print("\n测试5: nil值测试")
local nilValue = nil
if nilValue then
    print("✗ nil被认为是真")
else
    print("✓ nil被认为是假")
end

-- 测试6: 数字0测试（在Lua中0是真值）
print("\n测试6: 数字0测试")
local zero = 0
if zero then
    print("✓ 数字0被认为是真")
else
    print("✗ 数字0被认为是假")
end

-- 测试7: 空字符串测试（在Lua中空字符串是真值）
print("\n测试7: 空字符串测试")
local emptyStr = ""
if emptyStr then
    print("✓ 空字符串被认为是真")
else
    print("✗ 空字符串被认为是假")
end

print("\n=== 基础if语句测试完成 ===")
