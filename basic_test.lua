-- 基础功能测试

print("=== 基础测试开始 ===")

-- 1. 基本运算
local a = 10
local b = 20
print("加法:", a + b)
print("乘法:", a * b)

-- 2. 字符串
local str = "Hello World"
print("字符串:", str)

-- 3. 简单函数
function add(x, y)
    return x + y
end
print("函数调用:", add(5, 3))

-- 4. 多返回值
function multi()
    return 1, 2, 3
end
local x, y, z = multi()
print("多返回值:", x, y, z)

-- 5. 简单 vararg
function simple_vararg(...)
    return 42
end
print("简单 vararg:", simple_vararg(1, 2, 3))

print("=== 基础测试完成 ===")
