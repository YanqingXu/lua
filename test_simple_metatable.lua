-- 简单的元表测试
print("=== Simple Metatable Test ===")

-- 测试1: 检查函数是否存在
print("Test 1: Function availability")
print("setmetatable type:", type(setmetatable))
print("getmetatable type:", type(getmetatable))

-- 测试2: 创建表和元表
print("\nTest 2: Create tables")
local obj = {}
local meta = {}
print("obj created:", type(obj))
print("meta created:", type(meta))

-- 测试3: 设置元表
print("\nTest 3: Set metatable")
local result = setmetatable(obj, meta)
print("setmetatable result:", type(result))

-- 测试4: 获取元表
print("\nTest 4: Get metatable")
local retrieved = getmetatable(obj)
print("getmetatable result:", type(retrieved))
print("Same as original:", tostring(retrieved == meta))

-- 测试5: 简单的__index测试
print("\nTest 5: Simple __index test")
meta.__index = meta
meta.default_value = 42
print("meta.default_value:", meta.default_value)
print("obj.default_value:", obj.default_value)

print("\n=== Test completed ===")
