-- 测试注册机制的调试脚本
print("=== 测试注册机制调试 ===")

-- 1. 检查全局函数（BaseLib）
print("全局函数测试:")
print("type(print):", type(print))
print("type(type):", type(type))

-- 2. 检查表的存在
print("\n表存在性测试:")
print("type(string):", type(string))
print("type(math):", type(math))

-- 3. 详细检查表内容
print("\nstring表内容检查:")
if type(string) == "table" then
    print("string.len:", string.len)
    print("type(string.len):", type(string.len))
    
    print("string.sub:", string.sub)
    print("type(string.sub):", type(string.sub))
    
    print("string.upper:", string.upper)
    print("type(string.upper):", type(string.upper))
else
    print("string不是表类型!")
end

print("\nmath表内容检查:")
if type(math) == "table" then
    print("math.abs:", math.abs)
    print("type(math.abs):", type(math.abs))
    
    print("math.sin:", math.sin)
    print("type(math.sin):", type(math.sin))
    
    print("math.sqrt:", math.sqrt)
    print("type(math.sqrt):", type(math.sqrt))
else
    print("math不是表类型!")
end

-- 4. 尝试调用函数（如果存在）
print("\n函数调用测试:")
if string.len then
    local test_result = pcall(function() return string.len("hello") end)
    if test_result then
        print("string.len调用成功:", string.len("hello"))
    else
        print("string.len调用失败")
    end
else
    print("string.len为nil，无法调用")
end

if math.abs then
    local test_result = pcall(function() return math.abs(-5) end)
    if test_result then
        print("math.abs调用成功:", math.abs(-5))
    else
        print("math.abs调用失败")
    end
else
    print("math.abs为nil，无法调用")
end
