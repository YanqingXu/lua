-- 完整的可变参数测试
print("=== 完整可变参数测试 ===")

-- 测试1: 基本可变参数
print("\n1. 基本测试:")
local function test1(a, ...)
    print("固定参数 a:", a)
    local first_vararg = ...
    print("第一个可变参数:", first_vararg)
end
test1(10, 20, 30)

-- 测试2: 只有可变参数
print("\n2. 只有可变参数:")
local function test2(...)
    local first = ...
    print("第一个参数:", first)
end
test2(100, 200, 300)

-- 测试3: 无参数调用
print("\n3. 无参数调用:")
local function test3(...)
    local first = ...
    print("第一个参数:", first)
end
test3()

print("\n=== 测试完成 ===")
