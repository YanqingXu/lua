-- 表字面量详细调试
print("=== Table Literal Debug ===")

-- 测试1: 最简单的情况
print("Test 1: Single key-value")
local t1 = {x = 5}
print("t1.x =", tostring(t1.x))

-- 测试2: 两个键值对
print("\nTest 2: Two key-value pairs")
local t2 = {a = 1, b = 2}
print("t2.a =", tostring(t2.a))
print("t2.b =", tostring(t2.b))

-- 测试3: 对比手动构建
print("\nTest 3: Manual construction")
local t3 = {}
t3.a = 1
t3.b = 2
print("t3.a =", tostring(t3.a))
print("t3.b =", tostring(t3.b))

-- 测试4: 字符串键
print("\nTest 4: String keys")
local t4 = {name = "test"}
print("t4.name =", tostring(t4.name))

-- 测试5: 布尔值
print("\nTest 5: Boolean value")
local t5 = {flag = true}
print("t5.flag =", tostring(t5.flag))

print("\n=== Debug completed ===")
