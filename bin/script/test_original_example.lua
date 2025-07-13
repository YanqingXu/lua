-- 原始的可变参数例子
print("=== 原始可变参数例子测试 ===")

-- 定义一个使用可变参数的函数
local function sum(...)
    local total = 0
    local count = select("#", ...)
    print("参数数量:", count)
    
    for i = 1, count do
        local value = select(i, ...)
        print("参数", i, ":", value)
        if type(value) == "number" then
            total = total + value
        end
    end
    
    return total
end

-- 测试函数
print("\n测试 sum(1, 2, 3, 4, 5):")
local result = sum(1, 2, 3, 4, 5)
print("总和:", result)

print("\n测试 sum(10, 20):")
local result2 = sum(10, 20)
print("总和:", result2)

print("\n测试 sum():")
local result3 = sum()
print("总和:", result3)

print("\n=== 测试完成 ===")
