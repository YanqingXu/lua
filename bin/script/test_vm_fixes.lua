-- 测试VM指令修复效果
print("=== VM指令修复测试 ===")

-- 测试1: 比较指令的条件跳转语义
print("\n1. 测试比较指令的条件跳转语义:")

-- 测试EQ指令
print("测试 EQ (==) 指令:")
if 1 == 1 then
    print("  ✓ 1 == 1 正确")
else
    print("  ✗ 1 == 1 失败")
end

if 1 == 2 then
    print("  ✗ 1 == 2 错误地为真")
else
    print("  ✓ 1 == 2 正确为假")
end

-- 测试LT指令
print("测试 LT (<) 指令:")
if 1 < 2 then
    print("  ✓ 1 < 2 正确")
else
    print("  ✗ 1 < 2 失败")
end

if 2 < 1 then
    print("  ✗ 2 < 1 错误地为真")
else
    print("  ✓ 2 < 1 正确为假")
end

-- 测试LE指令
print("测试 LE (<=) 指令:")
if 1 <= 1 then
    print("  ✓ 1 <= 1 正确")
else
    print("  ✗ 1 <= 1 失败")
end

if 1 <= 2 then
    print("  ✓ 1 <= 2 正确")
else
    print("  ✗ 1 <= 2 失败")
end

if 2 <= 1 then
    print("  ✗ 2 <= 1 错误地为真")
else
    print("  ✓ 2 <= 1 正确为假")
end

-- 测试2: 字符串比较
print("\n2. 测试字符串比较:")
if "a" == "a" then
    print("  ✓ 字符串相等比较正确")
else
    print("  ✗ 字符串相等比较失败")
end

if "a" < "b" then
    print("  ✓ 字符串小于比较正确")
else
    print("  ✗ 字符串小于比较失败")
end

-- 测试3: 多值字符串连接 (CONCAT指令)
print("\n3. 测试多值字符串连接:")

-- 两个值连接
local s1 = "Hello"
local s2 = "World"
local result1 = s1 .. s2
print("  两值连接: '" .. s1 .. "' .. '" .. s2 .. "' = '" .. result1 .. "'")

-- 三个值连接
local s3 = "!"
local result2 = s1 .. s2 .. s3
print("  三值连接: '" .. s1 .. "' .. '" .. s2 .. "' .. '" .. s3 .. "' = '" .. result2 .. "'")

-- 测试4: 复杂的条件控制流
print("\n4. 测试复杂条件控制流:")

local x = 5
local y = 10

if x < y then
    print("  ✓ x < y 条件正确")
    if x == 5 then
        print("  ✓ 嵌套条件 x == 5 正确")
    else
        print("  ✗ 嵌套条件 x == 5 失败")
    end
else
    print("  ✗ x < y 条件失败")
end

-- 测试5: 循环中的比较
print("\n5. 测试循环中的比较:")
local count = 0
for i = 1, 3 do
    if i <= 3 then
        count = count + 1
    end
end
print("  循环计数结果: " .. count .. " (期望: 3)")

-- 测试6: 布尔值比较
print("\n6. 测试布尔值比较:")
local t = true
local f = false

if t == true then
    print("  ✓ true == true 正确")
else
    print("  ✗ true == true 失败")
end

if f == false then
    print("  ✓ false == false 正确")
else
    print("  ✗ false == false 失败")
end

if t == f then
    print("  ✗ true == false 错误地为真")
else
    print("  ✓ true == false 正确为假")
end

print("\n=== 测试完成 ===")
