-- 全面测试select函数
print("=== 全面测试select函数 ===")

-- 测试1: "#" 参数
print("\n1. 测试 '#' 参数:")
print("select('#'):", select("#"))
print("select('#', 1):", select("#", 1))
print("select('#', 1, 2, 3):", select("#", 1, 2, 3))
print("select('#', 'a', 'b', 'c', 'd'):", select("#", "a", "b", "c", "d"))

-- 测试2: 数字索引
print("\n2. 测试数字索引:")
print("select(1, 'a', 'b', 'c'):", select(1, "a", "b", "c"))
print("select(2, 'a', 'b', 'c'):", select(2, "a", "b", "c"))
print("select(3, 'a', 'b', 'c'):", select(3, "a", "b", "c"))

-- 测试3: 在函数中使用
print("\n3. 在函数中使用:")
local function test_varargs(...)
    print("参数个数:", select("#", ...))
    if select("#", ...) > 0 then
        print("第一个参数:", select(1, ...))
    end
    if select("#", ...) > 1 then
        print("第二个参数:", select(2, ...))
    end
end

test_varargs("hello", "world", 123)

print("\n=== 测试完成 ===")
