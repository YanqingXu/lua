-- test_arg_simple.lua - 简单测试 arg 表
print("Testing arg table...")

-- 测试 arg 是否存在
if arg then
    print("arg exists")
    print("arg type:", type(arg))
else
    print("arg is nil!")
end

-- 测试 arg[0]
if arg and arg[0] then
    print("arg[0] =", arg[0])
else
    print("arg[0] is nil or arg is nil")
end

print("Done")

