-- 测试可变参数的使用
print("开始测试可变参数使用")

local function test_func(...)
    print("函数内部开始")
    -- 不使用select，直接测试...的使用
    print("函数内部结束")
end

print("调用函数")
test_func(1, 2, 3)

print("测试完成")
