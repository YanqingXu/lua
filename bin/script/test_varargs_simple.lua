-- 测试可变参数语法
print("开始测试可变参数")

print("测试1: 定义可变参数函数")
local function test_func(...)
    print("函数被调用")
end

print("测试2: 调用可变参数函数")
test_func()

print("测试完成")
