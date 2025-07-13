-- 调试表元素访问问题
print("=== 调试表元素访问问题 ===")

-- 创建测试表
local t = {
    empty = {},
    nilval = nil,
    zero = 0,
    false_val = false,
    true_val = true,
    str = "test"
}

print("\n1. 直接访问测试:")
print("t.empty:", t.empty)
print("t.nilval:", t.nilval)  
print("t.zero:", t.zero)
print("t.false_val:", t.false_val)
print("t.true_val:", t.true_val)
print("t.str:", t.str)

print("\n2. 类型检查:")
print("type(t.empty):", type(t.empty))
print("type(t.nilval):", type(t.nilval))
print("type(t.zero):", type(t.zero))
print("type(t.false_val):", type(t.false_val))
print("type(t.true_val):", type(t.true_val))
print("type(t.str):", type(t.str))

print("\n3. 使用rawget测试:")
print("rawget(t, 'empty'):", rawget(t, "empty"))
print("rawget(t, 'nilval'):", rawget(t, "nilval"))
print("rawget(t, 'zero'):", rawget(t, "zero"))
print("rawget(t, 'false_val'):", rawget(t, "false_val"))
print("rawget(t, 'true_val'):", rawget(t, "true_val"))
print("rawget(t, 'str'):", rawget(t, "str"))

print("\n4. 条件判断测试:")
if t.zero then
    print("t.zero is truthy")
else
    print("t.zero is falsy")
end

if t.false_val then
    print("t.false_val is truthy")
else
    print("t.false_val is falsy")
end

if t.zero == 0 then
    print("t.zero equals 0")
else
    print("t.zero does not equal 0")
end

if t.false_val == false then
    print("t.false_val equals false")
else
    print("t.false_val does not equal false")
end

print("\n=== 调试完成 ===")
