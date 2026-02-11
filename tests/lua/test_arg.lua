-- test_arg.lua - 测试命令行参数 arg 表
-- 用法: lua.exe test_arg.lua hello world 123

print("=== arg table test ===")

-- 测试1: 脚本名
print("Script name:", arg[0])

-- 测试2: 遍历所有参数
print("Arguments:")
local i = 1
while arg[i] do
    print("  arg[" .. i .. "] =", arg[i])
    i = i + 1
end

-- 测试3: 表构造器（空表）
local t = {}
t[1] = "hello"
t[2] = "world"
print("t[1]:", t[1])
print("t[2]:", t[2])

print("=== done ===")

