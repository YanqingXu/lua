-- 最基础的表赋值测试
print("=== Basic Table Assignment Test ===")

-- 测试1: 空表 + 字符串键赋值
local t = {}
print("Created empty table")

-- 设置字符串键
t["name"] = "test"
print("Set t['name'] = 'test'")
print("t['name'] =", tostring(t["name"]))

-- 使用点语法
t.age = 25
print("Set t.age = 25")
print("t.age =", tostring(t.age))

-- 数字键
t[1] = "first"
print("Set t[1] = 'first'")
print("t[1] =", tostring(t[1]))

print("=== Basic test completed ===")
