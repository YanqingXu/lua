-- 测试基本的可变参数功能
print("=== 测试基本可变参数功能 ===")

-- 测试1: 获取第一个可变参数
print("\n1. 获取第一个可变参数:")
local function test1(...)
    local first = ...
    print("第一个参数:", first)
end
test1("hello", "world", 123)

-- 测试2: 在可变参数函数中使用多个参数
print("\n2. 多个参数测试:")
local function test2(a, b, ...)
    print("固定参数 a:", a)
    print("固定参数 b:", b)
    local first_vararg = ...
    print("第一个可变参数:", first_vararg)
end
test2(1, 2, 3, 4, 5)

-- 测试3: 无可变参数的情况
print("\n3. 无可变参数测试:")
local function test3(...)
    local first = ...
    print("第一个参数:", first)
end
test3()

print("\n=== 测试完成 ===")
