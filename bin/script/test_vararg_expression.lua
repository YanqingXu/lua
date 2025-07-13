-- 测试可变参数表达式
print("开始测试可变参数表达式")

local function test_func(...)
    print("函数内部开始")
    
    -- 测试使用...作为表达式（目前会返回nil，但不应该卡死）
    local args = ...
    print("args:", args)
    
    print("函数内部结束")
end

print("调用函数")
test_func(1, 2, 3)

print("测试完成")
