-- IO操作测试
-- 测试输入输出库的各种功能

print("=== IO操作测试 ===")

-- 测试1: 基本输出
print("测试1: 基本输出")
print("  这是print函数的输出")

-- 测试2: io.write函数
print("\n测试2: io.write函数")
if io and io.write then
    io.write("  这是io.write的输出\n")
    io.write("  多个", " ", "参数", " ", "输出\n")
else
    print("  io.write函数不可用")
end

-- 测试3: 输出不同类型的值
print("\n测试3: 输出不同类型的值")
local number = 42
local string = "测试字符串"
local boolean = true
local nilValue = nil

print("  数字:", number)
print("  字符串:", string)
print("  布尔值:", boolean)
print("  nil值:", nilValue)

-- 测试4: 格式化输出
print("\n测试4: 格式化输出")
local name = "张三"
local age = 25
local score = 95.5

if string and string.format then
    local formatted = string.format("姓名: %s, 年龄: %d, 分数: %.1f", name, age, score)
    print("  " .. formatted)
else
    print("  string.format不可用，使用连接:")
    print("  姓名: " .. name .. ", 年龄: " .. age .. ", 分数: " .. score)
end

-- 测试5: 表的输出
print("\n测试5: 表的输出")
local table1 = {a = 1, b = 2, c = 3}
print("  表对象:", table1)  -- 会显示表的地址

-- 自定义表打印函数
local function printTable(t, indent)
    indent = indent or ""
    for k, v in pairs(t) do
        if type(v) == "table" then
            print(indent .. k .. ":")
            printTable(v, indent .. "  ")
        else
            print(indent .. k .. ": " .. tostring(v))
        end
    end
end

print("  表内容:")
printTable(table1, "    ")

-- 测试6: 嵌套表的输出
print("\n测试6: 嵌套表的输出")
local nestedTable = {
    person = {
        name = "李四",
        age = 30,
        address = {
            city = "北京",
            district = "朝阳区"
        }
    },
    scores = {90, 85, 92}
}

print("  嵌套表内容:")
printTable(nestedTable, "    ")

-- 测试7: 数组的输出
print("\n测试7: 数组的输出")
local array = {10, 20, 30, 40, 50}
print("  数组长度:", #array)
print("  数组内容:")
for i = 1, #array do
    print("    [" .. i .. "] = " .. array[i])
end

-- 测试8: 函数的输出
print("\n测试8: 函数的输出")
local function testFunc()
    return "测试函数"
end

print("  函数对象:", testFunc)
print("  函数调用结果:", testFunc())

-- 测试9: 错误信息输出
print("\n测试9: 错误信息输出")
local success, error = pcall(function()
    error("这是一个测试错误")
end)

if not success then
    print("  捕获到错误:", error)
end

-- 测试10: 大量数据输出测试
print("\n测试10: 大量数据输出测试")
print("  输出1到10的平方:")
for i = 1, 10 do
    print("    " .. i .. "^2 = " .. (i * i))
end

-- 测试11: 特殊字符输出
print("\n测试11: 特殊字符输出")
print("  制表符:\t这里有制表符")
print("  换行符:\n这里有换行符")
print("  引号: \"双引号\" '单引号'")
print("  反斜杠: \\这是反斜杠\\")

-- 测试12: Unicode字符输出
print("\n测试12: Unicode字符输出")
print("  中文: 你好世界")
print("  日文: こんにちは")
print("  韩文: 안녕하세요")
print("  表情: 😀 🎉 ❤️")

-- 测试13: 数值的不同进制输出
print("\n测试13: 数值的不同进制输出")
local num = 255
if string and string.format then
    print("  十进制:", num)
    print("  十六进制:", string.format("%x", num))
    print("  八进制:", string.format("%o", num))
else
    print("  十进制:", num)
    print("  (其他进制需要string.format支持)")
end

print("\n=== IO操作测试完成 ===")
