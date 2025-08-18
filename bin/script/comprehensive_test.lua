-- 综合测试：验证所有功能
print("=== Lua解释器综合测试 ===")

-- 1. 基本算术运算
print("\n1. 基本算术运算:")
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

-- 4. 函数定义和调用
print("\n4. 函数定义和调用:")
function multiply(x, y)
    return x * y
end

local result = multiply(6, 7)
print("multiply(6, 7) =", result)

-- 5. 嵌套函数调用
print("\n5. 嵌套函数调用:")
function square(n)
    return n * n
end

function sumOfSquares(a, b)
    return square(a) + square(b)
end

local nested_result = sumOfSquares(3, 4)
print("sumOfSquares(3, 4) =", nested_result)

print("\n=== 所有测试完成 ===")
