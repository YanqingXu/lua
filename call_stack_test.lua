-- 调用栈测试脚本
-- 测试函数调用、嵌套调用、递归调用等功能

print("=== 调用栈功能测试 ===")

-- 测试1：简单函数调用
print("测试1：简单函数调用")
local function simple_func(x)
    return x * 2
end

local result1 = simple_func(5)
print("simple_func(5) =", result1)

-- 测试2：嵌套函数调用
print("\n测试2：嵌套函数调用")
local function add(a, b)
    return a + b
end

local function multiply(x, y)
    return x * y
end

local function nested_calc(a, b, c)
    return multiply(add(a, b), c)
end

local result2 = nested_calc(2, 3, 4)
print("nested_calc(2, 3, 4) =", result2)

-- 测试3：递归调用（阶乘）
print("\n测试3：递归调用")
local function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

local result3 = factorial(5)
print("factorial(5) =", result3)

-- 测试4：多返回值函数
print("\n测试4：多返回值函数")
local function multi_return(x)
    return x, x * 2, x * 3
end

local a, b, c = multi_return(4)
print("multi_return(4) =", a, b, c)

-- 测试5：函数作为参数
print("\n测试5：函数作为参数")
local function apply_func(func, value)
    return func(value)
end

local function square(x)
    return x * x
end

local result5 = apply_func(square, 6)
print("apply_func(square, 6) =", result5)

-- 测试6：局部变量作用域
print("\n测试6：局部变量作用域")
local function scope_test()
    local local_var = "inner"
    local function inner_func()
        return local_var
    end
    return inner_func()
end

local result6 = scope_test()
print("scope_test() =", result6)

-- 测试7：深度嵌套调用
print("\n测试7：深度嵌套调用")
local function deep_call(depth)
    if depth <= 0 then
        return "bottom"
    else
        return "level" .. depth .. "->" .. deep_call(depth - 1)
    end
end

local result7 = deep_call(3)
print("deep_call(3) =", result7)

print("\n=== 调用栈功能测试完成 ===")
