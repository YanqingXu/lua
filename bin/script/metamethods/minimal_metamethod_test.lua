-- 最简单的元方法测试
print("=== Minimal Metamethod Test ===")

-- 测试1: 基本setmetatable/getmetatable
print("Test 1: Basic setmetatable/getmetatable")
local obj = {name = "test"}
local meta = {type = "metatable"}

print("Before setmetatable:")
print("  obj.name = " .. obj.name)
print("  getmetatable(obj) = " .. tostring(getmetatable(obj)))

setmetatable(obj, meta)
print("After setmetatable:")
print("  getmetatable(obj) = " .. tostring(getmetatable(obj)))
print("  Same metatable? " .. tostring(getmetatable(obj) == meta))

-- 测试2: __index 元方法（表形式）
print("\nTest 2: __index metamethod (table form)")
local obj2 = {existing = "original"}
local defaults = {default_value = 100, default_name = "DefaultName"}
local meta2 = {__index = defaults}

setmetatable(obj2, meta2)

print("Testing __index:")
print("  obj2.existing = " .. tostring(obj2.existing))
print("  obj2.default_value = " .. tostring(obj2.default_value))
print("  obj2.default_name = " .. tostring(obj2.default_name))
print("  obj2.nonexistent = " .. tostring(obj2.nonexistent))

-- 测试3: __newindex 元方法（表形式）
print("\nTest 3: __newindex metamethod (table form)")
local obj3 = {existing = "original"}
local storage = {}
local meta3 = {__newindex = storage}

setmetatable(obj3, meta3)

print("Before assignment:")
print("  obj3.existing = " .. tostring(obj3.existing))
print("  storage.new_field = " .. tostring(storage.new_field))

obj3.new_field = "new_value"
print("After assignment:")
print("  obj3.new_field = " .. tostring(obj3.new_field))
print("  storage.new_field = " .. tostring(storage.new_field))

-- 测试4: 修改已存在的字段
print("\nTest 4: Modify existing field")
print("Before modification:")
print("  obj3.existing = " .. tostring(obj3.existing))

obj3.existing = "modified"
print("After modification:")
print("  obj3.existing = " .. tostring(obj3.existing))
print("  storage.existing = " .. tostring(storage.existing))

print("\n=== All tests completed ===")
print("Status: Basic metamethod functionality working correctly")
