-- 测试可变参数解析
print("开始测试可变参数解析")

local function test_func(...)
    print("函数内部开始")
    -- 现在应该不会卡死了
    print("函数内部结束")
end

print("调用函数")
test_func(1, 2, 3)

print("测试完成")
