-- 简单的空字符串测试

print("=== Simple Empty String Test ===")

-- 直接测试空字符串
local empty = ""
print("empty =", empty)
print("type(empty) =", type(empty))

-- 测试字符串连接
local result = empty .. "hello"
print("result =", result)

print("=== Test End ===")
