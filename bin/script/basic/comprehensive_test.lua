-- 综合测试：验证VM各项功能正常工作

print("=== 基础功能测试 ===")

-- 1. 基本变量和运算
local a = 10
local b = 20
print("基本运算:", a + b, a * b)

-- 2. 字符串操作
local str1 = "Hello"
local str2 = "World"
print("字符串连接:", str1 .. " " .. str2)

-- 3. 表操作
local t = {1, 2, 3}
t[4] = 4
print("表操作:", t[1], t[2], t[3], t[4])

print("\n=== 函数调用测试 ===")

-- 4. 简单函数调用
function add(x, y)
    return x + y
end
print("简单函数:", add(5, 3))

-- 5. 多返回值函数
function multi()
    return 1, 2, 3
end
local x, y, z = multi()
print("多返回值:", x, y, z)

-- 6. 嵌套函数调用
function double(n)
    return n * 2
end
print("嵌套调用:", add(double(5), 3))

print("\n=== Vararg 函数测试 ===")

-- 7. Vararg 函数
function sum(...)
    local total = 0
    local args = {...}
    for i = 1, #args do
        total = total + args[i]
    end
    return total
end
print("Vararg 求和:", sum(1, 2, 3, 4, 5))

-- 8. Vararg 传递
function pass(...)
    return ...
end
print("Vararg 传递:", pass(10, 20, 30))

-- 9. 复杂的 vararg 场景
function complex_vararg(...)
    return pass(...)
end
print("复杂 vararg:", complex_vararg(100, 200))

print("\n=== 控制流测试 ===")

-- 10. 条件语句
if a > b then
    print("条件测试: a > b")
else
    print("条件测试: a <= b")
end

-- 11. 循环
local sum_result = 0
for i = 1, 5 do
    sum_result = sum_result + i
end
print("循环测试:", sum_result)

print("\n=== 所有测试完成 ===")
