-- 测试在函数内部使用select
print("开始测试")

local function test_func(...)
    print("函数内部开始")
    local count = select("#", ...)
    print("参数个数:", count)
    print("函数内部结束")
end

print("调用函数")
test_func(1, 2, 3)

print("测试完成")
