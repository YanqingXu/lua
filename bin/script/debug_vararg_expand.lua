-- 测试可变参数展开
print("测试可变参数展开")

local function test(...)
    print("直接调用select:")
    print("select('#', 1, 2, 3) =", select("#", 1, 2, 3))
    
    print("通过可变参数调用select:")
    print("select('#', ...) =", select("#", ...))
end

test(1, 2, 3)
