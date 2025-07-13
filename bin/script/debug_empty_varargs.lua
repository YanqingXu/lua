-- 调试无参数可变参数的问题
print("调试无参数可变参数")

local function test_empty(...)
    print("开始测试空可变参数")
    local count = select("#", ...)
    print("参数数量:", count)
    
    if count > 0 then
        print("第1个参数:", select(1, ...))
        print("第2个参数:", select(2, ...))
        print("第3个参数:", select(3, ...))
    end
    
    print("测试结束")
end

print("调用 test_empty()")
test_empty()

print("调试完成")
