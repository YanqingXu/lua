-- 测试高级Lua功能
print("=== 高级Lua功能测试 ===")

-- 测试变量和运算
local a = 10
local b = 20
print("a + b =", a + b)
print("a * b =", a * b)

-- 测试字符串操作
local str1 = "Hello"
local str2 = "World"
print("字符串连接:", str1 .. " " .. str2)

-- 测试表操作
local table1 = {1, 2, 3, 4, 5}
print("表元素:", table1[1], table1[2], table1[3])

-- 测试函数
function add(x, y)
    return x + y
end

print("函数调用:", add(15, 25))

-- 测试条件语句
if a < b then
    print("a 小于 b")
else
    print("a 大于等于 b")
end

-- 测试循环
print("循环测试:")
for i = 1, 5 do
    print("  i =", i)
end

print("=== 所有测试完成 ===")
