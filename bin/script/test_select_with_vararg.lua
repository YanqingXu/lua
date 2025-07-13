-- 测试select与可变参数的组合
print("开始测试select与可变参数")

local function test_func(...)
    print("函数内部开始")
    
    -- 这个调用之前会导致卡死，现在应该不会了
    print("准备调用select('#', ...)")
    local count = select("#", ...)
    print("参数个数:", count)
    
    print("函数内部结束")
end

print("调用函数")
test_func(1, 2, 3)

print("测试完成")
