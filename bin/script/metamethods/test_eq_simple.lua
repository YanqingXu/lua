-- 简单的等于比较测试
print("=== Simple Equality Test ===")

-- 测试基础比较
local a = 42
local b = 42
local c = 43

print("Testing basic equality...")
print("a =", a)
print("b =", b) 
print("c =", c)

-- 这里可能会出错，让我们看看
local result1 = (a == b)
print("a == b:", result1)

local result2 = (a == c)
print("a == c:", result2)
