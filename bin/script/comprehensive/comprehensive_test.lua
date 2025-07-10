-- 综合功能测试

print("=== Comprehensive Test Start ===")

-- 测试1：变量和基本运算
print("Test 1: Variables and arithmetic")
local a = 10
local b = 3
print("a + b =", a + b)
print("a * b =", a * b)

-- 测试2：函数定义和调用
print("Test 2: Function definition and call")
function add(x, y)
    return x + y
end

local result = add(5, 7)
print("add(5, 7) =", result)

-- 测试3：字符串操作
print("Test 3: String operations")
local name = "Lua"
local greeting = "Hello, " .. name .. "!"
print("greeting =", greeting)

-- 测试4：条件语句
print("Test 4: Conditional statements")
if a > b then
    print("a is greater than b")
else
    print("a is not greater than b")
end

-- 测试5：循环
print("Test 5: Loops")
print("Counting from 1 to 3:")
for i = 1, 3 do
    print("  i =", i)
end

print("=== Comprehensive Test End ===")
