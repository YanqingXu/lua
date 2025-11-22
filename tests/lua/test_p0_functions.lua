-- P0修复验证测试

-- 测试1: 简单函数调用
function add(a, b)
    return a + b
end

local result1 = add(3, 4)

-- 测试2: 嵌套函数调用
function mul(a, b)
    return a * b
end

function calc(x, y)
    return mul(x, y)
end

local result2 = calc(5, 6)

-- 测试3: 递归调用
function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

local result3 = factorial(5)

-- 测试4: 闭包
function makeCounter()
    local count = 0
    return function()
        count = count + 1
        return count
    end
end

local counter = makeCounter()
local c1 = counter()
local c2 = counter()
local c3 = counter()

