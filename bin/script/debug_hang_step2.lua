-- 逐步测试找出卡死原因 - 第2步
print("步骤2开始")

print("测试1: select函数")
local function test_select()
    print("select('#', 1, 2, 3):", select("#", 1, 2, 3))
end
test_select()

print("测试2: 可变参数函数")
local function test_varargs(...)
    print("参数个数:", select("#", ...))
    return ...
end
test_varargs(1, 2, 3)

print("测试3: pcall函数")
local success, result = pcall(function()
    return "pcall测试"
end)
print("pcall成功:", success, "结果:", result)

print("步骤2完成")
