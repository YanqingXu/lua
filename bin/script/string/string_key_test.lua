-- 测试字符串键编译
print("=== String Key Compilation Test ===")

-- 直接测试字符串作为键
local t = {}
local key = "name"
local value = "test"

t[key] = value
print("Dynamic assignment: t[key] = value")
print("t[key] =", tostring(t[key]))

-- 测试字面量字符串键
t["direct"] = "direct_value"
print("Direct string key: t['direct'] = 'direct_value'")
print("t['direct'] =", tostring(t["direct"]))

print("=== String key test completed ===")
