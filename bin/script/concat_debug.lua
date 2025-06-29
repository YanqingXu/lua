-- 字符串连接调试测试

print("=== String Concatenation Debug ===")

-- 测试1：定义变量
local PROGRAM_NAME = "Lua Calculator & Demo"
local VERSION = "1.0"

print("PROGRAM_NAME =", PROGRAM_NAME)
print("VERSION =", VERSION)

-- 测试2：简单连接
print("Simple concat test:")
local result1 = "Hello " .. "World"
print("result1 =", result1)

-- 测试3：变量连接
print("Variable concat test:")
local result2 = PROGRAM_NAME .. " v" .. VERSION
print("result2 =", result2)

-- 测试4：复杂连接（类似原始代码）
print("Complex concat test:")
local result3 = " " .. PROGRAM_NAME .. " v" .. VERSION
print("result3 =", result3)

-- 测试5：在函数中
function testFunction()
    print("In function:")
    print("PROGRAM_NAME =", PROGRAM_NAME)
    print("VERSION =", VERSION)
    local result = " " .. PROGRAM_NAME .. " v" .. VERSION
    print("result =", result)
end

testFunction()

print("=== Debug End ===")
