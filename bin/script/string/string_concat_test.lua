-- 最基本的字符串连接测试
print("=== String Concatenation Test ===")

-- 测试1: 字符串字面量连接
print("Test 1: String literal concatenation")
local s1 = "Hello"
local s2 = "World"
print("s1 = " .. s1)
print("s2 = " .. s2)
local result = s1 .. " " .. s2
print("Result: " .. result)

-- 测试2: 数字连接
print("\nTest 2: Number concatenation")
local n1 = 10
local n2 = 20
print("n1 = " .. n1)
print("n2 = " .. n2)
local num_result = n1 .. " + " .. n2
print("Result: " .. num_result)

print("\n=== String concatenation test completed ===")
