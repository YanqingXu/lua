-- 第二阶段重构验证脚本
-- 验证CallInfo增强、CallStack类、FunctionCall模块功能

print("=== 第二阶段重构验证 ===")

-- 测试CallInfo增强功能（通过函数调用间接测试）
print("测试1：CallInfo增强功能")
local function test_callinfo()
    local x = 10
    local y = 20
    return x + y
end

local result1 = test_callinfo()
print("CallInfo test result:", result1)

-- 测试CallStack管理（通过嵌套调用测试）
print("\n测试2：CallStack管理")
local function level3()
    return "level3"
end

local function level2()
    return "level2->" .. level3()
end

local function level1()
    return "level1->" .. level2()
end

local result2 = level1()
print("CallStack test result:", result2)

-- 测试FunctionCall优化（通过不同类型的调用测试）
print("\n测试3：FunctionCall优化")

-- 简单调用
local function simple(a, b)
    return a * b
end

-- 带默认参数的调用
local function with_defaults(a, b, c)
    c = c or 1
    return a + b + c
end

-- 可变参数调用
local function varargs(...)
    local sum = 0
    for i = 1, select('#', ...) do
        sum = sum + select(i, ...)
    end
    return sum
end

local result3a = simple(3, 4)
local result3b = with_defaults(1, 2)
local result3c = varargs(1, 2, 3, 4, 5)

print("Simple call:", result3a)
print("Default args call:", result3b)
print("Varargs call:", result3c)

-- 测试错误处理和恢复
print("\n测试4：错误处理")
local function safe_divide(a, b)
    if b == 0 then
        return nil, "Division by zero"
    else
        return a / b
    end
end

local result4a, err4a = safe_divide(10, 2)
local result4b, err4b = safe_divide(10, 0)

print("Safe divide 10/2:", result4a, err4a)
print("Safe divide 10/0:", result4b, err4b)

-- 测试尾调用优化（间接测试）
print("\n测试5：尾调用优化")
local function tail_recursive(n, acc)
    acc = acc or 0
    if n <= 0 then
        return acc
    else
        return tail_recursive(n - 1, acc + n)
    end
end

local result5 = tail_recursive(10)
print("Tail recursive sum(1..10):", result5)

print("\n=== 第二阶段重构验证完成 ===")
