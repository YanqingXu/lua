-- 测试高级可变参数功能
print("=== 高级可变参数测试 ===")

-- 测试1: 可变参数函数调用另一个可变参数函数
print("\n1. 嵌套可变参数调用:")
local function inner(...)
    return select("#", ...)
end

local function outer(...)
    local count = inner(...)
    print("内层函数收到参数数量:", count)
    return count
end

local result1 = outer("a", "b", "c", "d")
print("外层函数返回:", result1)

-- 测试2: 可变参数与固定参数混合
print("\n2. 混合参数测试:")
local function mixed(first, second, ...)
    print("固定参数1:", first)
    print("固定参数2:", second)
    local vararg_count = select("#", ...)
    print("可变参数数量:", vararg_count)
    if vararg_count > 0 then
        print("第1个可变参数:", select(1, ...))
    end
end

mixed("hello", "world", 1, 2, 3)

-- 测试3: 可变参数传递给print
print("\n3. 可变参数传递给print:")
local function print_all(...)
    print("参数:", ...)
end

print_all("test", 123, true)

print("\n=== 测试完成 ===")
