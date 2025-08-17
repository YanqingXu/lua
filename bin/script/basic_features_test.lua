-- 基本功能测试（不包含函数调用）
print("=== 基本功能测试 ===")

-- 1. 变量和算术运算
print("\n1. 变量和算术运算:")
local a = 10
local b = 5
print("a =", a)
print("b =", b)
print("a + b =", a + b)
print("a - b =", a - b)
print("a * b =", a * b)
print("a / b =", a / b)

-- 2. 条件语句
print("\n2. 条件语句:")
if a > b then
    print("a大于b")
else
    print("a不大于b")
end

-- 3. for循环
print("\n3. for循环:")
local sum = 0
for i = 1, 5 do
    sum = sum + i
    print("i =", i, "sum =", sum)
end
print("最终sum =", sum)

print("\n=== 基本功能测试完成 ===")
