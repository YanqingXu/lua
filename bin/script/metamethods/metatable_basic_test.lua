-- 元表基本功能测试
print("=== Basic Metatable Test ===")

-- 测试1: 创建表和元表
print("Test 1: Create table and metatable")
local t = {}
local mt = {}
print("Table created: " .. tostring(t))
print("Metatable created: " .. tostring(mt))

-- 测试2: 设置元表
print("\nTest 2: Set metatable")
setmetatable(t, mt)
local retrieved_mt = getmetatable(t)
print("Retrieved metatable: " .. tostring(retrieved_mt))
print("Same as original: " .. tostring(retrieved_mt == mt))

-- 测试3: 添加__index元方法
print("\nTest 3: Add __index metamethod")
local defaults = {x = 100, y = 200}
mt.__index = defaults
print("__index set to: " .. tostring(mt.__index))

-- 测试4: 测试__index
print("\nTest 4: Test __index")
print("t.x = " .. tostring(t.x))
print("t.y = " .. tostring(t.y))
print("t.z = " .. tostring(t.z))

-- 测试5: 设置字段
print("\nTest 5: Set field")
t.a = 50
print("t.a = " .. tostring(t.a))

print("\n=== Basic metatable test completed ===")
