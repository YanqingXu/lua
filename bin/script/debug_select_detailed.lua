-- 详细调试select在可变参数函数中的问题
print("开始详细调试")

local function test_func(...)
    print("进入函数")
    print("准备调用select")
    
    -- 尝试最简单的调用
    print("调用select('#', 1)")
    local result = select("#", 1)
    print("select结果:", result)
    
    print("函数结束")
end

print("准备调用函数")
test_func(1, 2, 3)
print("函数调用完成")

print("调试完成")
