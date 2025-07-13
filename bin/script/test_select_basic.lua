-- 测试select函数的基本使用
print("=== 测试select函数基本使用 ===")

-- 测试1: select("#", ...)
print("\n1. 测试select('#', ...):")
local function test_count(...)
    local count = select("#", ...)
    print("参数数量:", count)
end
test_count(1, 2, 3, 4, 5)

-- 测试2: select(n, ...)
print("\n2. 测试select(n, ...):")
local function test_select(...)
    print("第1个参数:", select(1, ...))
    print("第2个参数:", select(2, ...))
    print("第3个参数:", select(3, ...))
end
test_select("hello", "world", "lua", "test")

-- 测试3: 无参数情况
print("\n3. 测试无参数情况:")
local function test_empty(...)
    local count = select("#", ...)
    print("参数数量:", count)
end
test_empty()

print("\n=== 测试完成 ===")
