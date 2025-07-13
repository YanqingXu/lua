-- 测试在可变参数函数中使用...作为select参数
print("开始测试")

local function test_func(...)
    print("进入函数")
    print("准备调用select('#', ...)")
    
    -- 这里可能是问题所在
    local result = select("#", ...)
    print("select结果:", result)
    
    print("函数结束")
end

print("准备调用函数")
test_func(1, 2, 3)
print("函数调用完成")

print("测试完成")
