-- 简单测试文件，逐步诊断问题

print("=== Simple Test Start ===")

-- 测试1：基本变量赋值
print("Test 1: Basic variable assignment")
local a = 10
local b = 3
print("a =", a)
print("b =", b)

-- 测试2：简单函数定义
print("Test 2: Simple function definition")
function add(x, y)
    return x + y
end

-- 测试3：函数调用
print("Test 3: Function call")
local result = add(a, b)
print("add(a, b) =", result)

print("=== Simple Test End ===")
