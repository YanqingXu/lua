-- 逐步测试找出卡死原因 - 第1步
print("步骤1: 基本print测试")
print("这是一个简单的print")

print("步骤2: 变量赋值测试")
local x = 10
print("x =", x)

print("步骤3: 表创建测试")
local t = {}
print("表创建成功")

print("步骤4: 函数定义测试")
local function test_func()
    return "函数调用成功"
end
print("函数定义成功")

print("步骤5: 函数调用测试")
local result = test_func()
print("函数调用结果:", result)

print("步骤1完成")
