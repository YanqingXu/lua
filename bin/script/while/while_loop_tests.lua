-- while循环测试
-- 测试while和repeat-until循环

print("=== while循环测试 ===")

-- 测试1: 基本while循环
print("测试1: 基本while循环 (计数到5)")
local i = 1
while i <= 5 do
    print("  i =", i)
    i = i + 1
end

-- 测试2: while循环条件为false（不执行）
print("\n测试2: while循环条件为false")
local executed = false
while false do
    executed = true
    print("  这不应该被打印")
end
if not executed then
    print("✓ 条件为false的while循环正确地没有执行")
else
    print("✗ 条件为false的while循环错误地执行了")
end

-- 测试3: while循环中的break
print("\n测试3: while循环中的break")
local j = 1
while true do
    if j > 3 then
        print("  在j=" .. j .. "处break")
        break
    end
    print("  j =", j)
    j = j + 1
end

-- 测试4: 嵌套while循环
print("\n测试4: 嵌套while循环")
local outer = 1
while outer <= 2 do
    print("  外层循环: outer =", outer)
    local inner = 1
    while inner <= 3 do
        print("    内层循环: inner =", inner)
        inner = inner + 1
    end
    outer = outer + 1
end

-- 测试5: while循环计算阶乘
print("\n测试5: while循环计算阶乘")
local n = 5
local factorial = 1
local temp = n
while temp > 0 do
    factorial = factorial * temp
    temp = temp - 1
end
print("  " .. n .. "! =", factorial)
if factorial == 120 then
    print("✓ 阶乘计算正确")
else
    print("✗ 阶乘计算错误")
end

print("\n=== repeat-until循环测试 ===")

-- 测试6: 基本repeat-until循环
print("测试6: 基本repeat-until循环")
local k = 1
repeat
    print("  k =", k)
    k = k + 1
until k > 3

-- 测试7: repeat-until至少执行一次
print("\n测试7: repeat-until至少执行一次")
local executed2 = false
repeat
    executed2 = true
    print("  repeat-until至少执行一次")
until true
if executed2 then
    print("✓ repeat-until正确地至少执行了一次")
else
    print("✗ repeat-until没有执行")
end

-- 测试8: repeat-until中的break
print("\n测试8: repeat-until中的break")
local m = 1
repeat
    if m == 3 then
        print("  在m=" .. m .. "处break")
        break
    end
    print("  m =", m)
    m = m + 1
until false

-- 测试9: 复杂条件的repeat-until
print("\n测试9: 复杂条件的repeat-until")
local sum = 0
local num = 1
repeat
    sum = sum + num
    print("  num=" .. num .. ", sum=" .. sum)
    num = num + 1
until sum > 10 or num > 5

-- 测试10: 嵌套repeat-until
print("\n测试10: 嵌套repeat-until")
local x = 1
repeat
    print("  外层: x =", x)
    local y = 1
    repeat
        print("    内层: y =", y)
        y = y + 1
    until y > 2
    x = x + 1
until x > 2

-- 测试11: while和repeat-until的区别
print("\n测试11: while和repeat-until的区别")
print("while循环（条件为false）:")
local count1 = 0
while false do
    count1 = count1 + 1
end
print("  执行次数:", count1)

print("repeat-until循环（条件为true）:")
local count2 = 0
repeat
    count2 = count2 + 1
until true
print("  执行次数:", count2)

print("\n=== while循环测试完成 ===")
