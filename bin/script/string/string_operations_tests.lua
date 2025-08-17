-- 字符串操作测试
-- 测试字符串库的各种功能

print("=== 字符串操作测试 ===")

-- 测试1: 字符串长度
print("测试1: 字符串长度")
local str1 = "Hello"
local str2 = "你好世界"
local str3 = ""
print("  #'Hello' =", #str1)
print("  #'你好世界' =", #str2)
print("  #'' =", #str3)

-- 测试2: 字符串连接
print("\n测试2: 字符串连接")
local part1 = "Hello"
local part2 = " "
local part3 = "World"
local combined = part1 .. part2 .. part3
print("  'Hello' .. ' ' .. 'World' =", combined)

-- 测试3: 字符串比较
print("\n测试3: 字符串比较")
local a = "apple"
local b = "banana"
local c = "apple"
print("  'apple' == 'banana':", a == b)
print("  'apple' == 'apple':", a == c)
print("  'apple' < 'banana':", a < b)
print("  'banana' > 'apple':", b > a)

-- 测试4: string.len函数
print("\n测试4: string.len函数")
if string and string.len then
    print("  string.len('Hello') =", string.len("Hello"))
    print("  string.len('') =", string.len(""))
else
    print("  string.len函数不可用")
end

-- 测试5: string.upper和string.lower
print("\n测试5: string.upper和string.lower")
if string and string.upper and string.lower then
    local text = "Hello World"
    print("  原文:", text)
    print("  string.upper(text) =", string.upper(text))
    print("  string.lower(text) =", string.lower(text))
else
    print("  string.upper/lower函数不可用")
end

-- 测试6: string.sub
print("\n测试6: string.sub")
if string and string.sub then
    local text = "Hello World"
    print("  原文:", text)
    print("  string.sub(text, 1, 5) =", string.sub(text, 1, 5))
    print("  string.sub(text, 7) =", string.sub(text, 7))
    print("  string.sub(text, -5) =", string.sub(text, -5))
else
    print("  string.sub函数不可用")
end

-- 测试7: 字符串和数字的自动转换
print("\n测试7: 字符串和数字的自动转换")
local numStr = "123"
local strNum = 456
print("  '123' + 1 =", numStr + 1)
print("  456 .. ' test' =", strNum .. " test")

-- 测试8: 多行字符串
print("\n测试8: 多行字符串")
local multiline = [[这是一个
多行字符串
测试]]
print("  多行字符串:")
print(multiline)

-- 测试9: 转义字符
print("\n测试9: 转义字符")
local escaped = "第一行\n第二行\t制表符\\"反斜杠\\""
print("  转义字符测试:")
print(escaped)

-- 测试10: 字符串重复
print("\n测试10: 字符串重复")
if string and string.rep then
    print("  string.rep('*', 5) =", string.rep("*", 5))
    print("  string.rep('abc', 3) =", string.rep("abc", 3))
else
    print("  string.rep函数不可用")
end

-- 测试11: 字符串查找
print("\n测试11: 字符串查找")
if string and string.find then
    local text = "Hello World Hello"
    print("  原文:", text)
    local pos = string.find(text, "World")
    print("  string.find(text, 'World') =", pos)
    local pos2 = string.find(text, "Hello", 2)
    print("  string.find(text, 'Hello', 2) =", pos2)
else
    print("  string.find函数不可用")
end

-- 测试12: 字符串格式化
print("\n测试12: 字符串格式化")
if string and string.format then
    local name = "张三"
    local age = 25
    local formatted = string.format("姓名: %s, 年龄: %d", name, age)
    print("  格式化结果:", formatted)
else
    print("  string.format函数不可用")
end

-- 测试13: 字符串反转
print("\n测试13: 字符串反转")
if string and string.reverse then
    local text = "Hello"
    print("  string.reverse('Hello') =", string.reverse(text))
else
    print("  string.reverse函数不可用")
end

print("\n=== 字符串操作测试完成 ===")
