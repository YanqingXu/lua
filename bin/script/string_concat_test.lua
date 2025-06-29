-- 字符串连接测试

print("=== String Concatenation Test ===")

-- 测试变量
local PROGRAM_NAME = "Lua Calculator & Demo"
local VERSION = "1.0"

-- 测试1: 简单字符串连接
print("Test 1: Simple concatenation")
local result1 = "Hello" .. " " .. "World"
print("result1 =", result1)

-- 测试2: 变量字符串连接
print("Test 2: Variable concatenation")
local result2 = PROGRAM_NAME .. " v" .. VERSION
print("result2 =", result2)

-- 测试3: 包含空格的连接（问题场景）
print("Test 3: Space prefix concatenation")
local result3 = " " .. PROGRAM_NAME .. " v" .. VERSION
print("result3 =", result3)

-- 测试4: 分步连接
print("Test 4: Step by step concatenation")
local step1 = " " .. PROGRAM_NAME
print("step1 =", step1)
local step2 = step1 .. " v"
print("step2 =", step2)
local step3 = step2 .. VERSION
print("step3 =", step3)

print("=== Test End ===")
