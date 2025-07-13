-- 测试修复后的raw函数
print("=== 测试修复后的raw函数 ===")

-- 创建测试表
local t = {
    str = "hello",
    num = 42,
    bool = true,
    empty = {},
    nilval = nil
}

print("\n1. rawget 测试:")
print("rawget(t, 'str'):", rawget(t, "str"))
print("rawget(t, 'num'):", rawget(t, "num"))
print("rawget(t, 'bool'):", rawget(t, "bool"))
print("rawget(t, 'empty'):", rawget(t, "empty"))
print("rawget(t, 'nilval'):", rawget(t, "nilval"))
print("rawget(t, 'nonexistent'):", rawget(t, "nonexistent"))

print("\n2. rawset 测试:")
local t2 = {}
print("设置前 t2.test:", rawget(t2, "test"))
rawset(t2, "test", "value")
print("设置后 t2.test:", rawget(t2, "test"))
rawset(t2, "number", 123)
print("t2.number:", rawget(t2, "number"))

print("\n3. rawlen 测试:")
local str = "hello world"
local arr = {1, 2, 3, 4, 5}
print("rawlen(str):", rawlen(str))
print("rawlen(arr):", rawlen(arr))

print("\n4. rawequal 测试:")
local a = "test"
local b = "test"
local c = "different"
local d = 42
local e = 42
local f = true
local g = false

print("rawequal(a, b):", rawequal(a, b))
print("rawequal(a, c):", rawequal(a, c))
print("rawequal(d, e):", rawequal(d, e))
print("rawequal(f, g):", rawequal(f, g))
print("rawequal(a, d):", rawequal(a, d))

-- 测试表引用相等性
local t3 = {}
local t4 = t3
local t5 = {}
print("rawequal(t3, t4):", rawequal(t3, t4))
print("rawequal(t3, t5):", rawequal(t3, t5))

print("\n5. 与元表交互测试:")
local mt = {
    __index = function(t, k)
        return "from_metamethod_" .. k
    end
}
local obj = {}
setmetatable(obj, mt)

print("普通访问 obj.test:", obj.test)
print("rawget(obj, 'test'):", rawget(obj, "test"))

rawset(obj, "direct", "direct_value")
print("设置后 obj.direct:", obj.direct)
print("rawget(obj, 'direct'):", rawget(obj, "direct"))

print("\n=== 测试完成 ===")
