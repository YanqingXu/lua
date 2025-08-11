-- 简化的 vararg 测试

print("=== 简单 vararg 测试 ===")

-- 测试1：基本 vararg 函数
function test1(...)
    print("test1 called with varargs")
    return 42
end

local result = test1(1, 2, 3)
print("test1 result:", result)

-- 测试2：vararg 传递
function pass(...)
    return ...
end

local a, b = pass(10, 20)
print("pass result:", a, b)

print("=== 测试完成 ===")
