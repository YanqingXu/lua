-- 基础函数测试
-- 测试函数定义、调用、参数传递等基本功能

print("=== 基础函数测试 ===")

-- 测试1: 简单函数定义和调用
print("测试1: 简单函数定义和调用")
function greet()
    return "Hello, World!"
end

local message = greet()
print("  greet() =", message)

-- 测试2: 带参数的函数
print("\n测试2: 带参数的函数")
function add(a, b)
    return a + b
end

local sum = add(3, 5)
print("  add(3, 5) =", sum)

-- 测试3: 多个参数的函数
print("\n测试3: 多个参数的函数")
function multiply(a, b, c)
    return a * b * c
end

local product = multiply(2, 3, 4)
print("  multiply(2, 3, 4) =", product)

-- 测试4: 可变参数函数
print("\n测试4: 可变参数函数")
function sum(...)
    local args = {...}
    local total = 0
    for i = 1, #args do
        total = total + args[i]
    end
    return total
end

local result1 = sum(1, 2, 3)
local result2 = sum(1, 2, 3, 4, 5)
print("  sum(1, 2, 3) =", result1)
print("  sum(1, 2, 3, 4, 5) =", result2)

-- 测试5: 多返回值函数
print("\n测试5: 多返回值函数")
function divmod(a, b)
    return a // b, a % b
end

local quotient, remainder = divmod(17, 5)
print("  divmod(17, 5) =", quotient, remainder)

-- 测试6: 局部函数
print("\n测试6: 局部函数")
local function localFunction(x)
    return x * x
end

local square = localFunction(7)
print("  localFunction(7) =", square)

-- 测试7: 匿名函数
print("\n测试7: 匿名函数")
local anonymous = function(x, y)
    return x + y
end

local result3 = anonymous(10, 20)
print("  anonymous(10, 20) =", result3)

-- 测试8: 函数作为参数
print("\n测试8: 函数作为参数")
function applyFunction(func, x, y)
    return func(x, y)
end

local result4 = applyFunction(add, 15, 25)
print("  applyFunction(add, 15, 25) =", result4)

-- 测试9: 函数作为返回值
print("\n测试9: 函数作为返回值")
function createMultiplier(factor)
    return function(x)
        return x * factor
    end
end

local double = createMultiplier(2)
local triple = createMultiplier(3)
print("  double(5) =", double(5))
print("  triple(5) =", triple(5))

-- 测试10: 递归函数
print("\n测试10: 递归函数")
function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

local fact5 = factorial(5)
print("  factorial(5) =", fact5)

-- 测试11: 尾递归优化测试
print("\n测试11: 尾递归函数")
function tailFactorial(n, acc)
    acc = acc or 1
    if n <= 1 then
        return acc
    else
        return tailFactorial(n - 1, n * acc)
    end
end

local tailFact5 = tailFactorial(5)
print("  tailFactorial(5) =", tailFact5)

-- 测试12: 函数的默认参数模拟
print("\n测试12: 函数的默认参数模拟")
function greetWithDefault(name, greeting)
    name = name or "World"
    greeting = greeting or "Hello"
    return greeting .. ", " .. name .. "!"
end

print("  greetWithDefault() =", greetWithDefault())
print("  greetWithDefault('Alice') =", greetWithDefault("Alice"))
print("  greetWithDefault('Bob', 'Hi') =", greetWithDefault("Bob", "Hi"))

print("\n=== 基础函数测试完成 ===")
