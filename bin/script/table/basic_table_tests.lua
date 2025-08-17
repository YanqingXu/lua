-- 基础表操作测试
-- 测试表的创建、访问、修改等基本操作

print("=== 基础表操作测试 ===")

-- 测试1: 空表创建
print("测试1: 空表创建")
local emptyTable = {}
print("✓ 空表创建成功")

-- 测试2: 数组式表创建
print("\n测试2: 数组式表创建")
local array = {1, 2, 3, 4, 5}
print("  array[1] =", array[1])
print("  array[3] =", array[3])
print("  array[5] =", array[5])

-- 测试3: 字典式表创建
print("\n测试3: 字典式表创建")
local dict = {
    name = "张三",
    age = 25,
    city = "北京"
}
print("  dict.name =", dict.name)
print("  dict.age =", dict.age)
print("  dict.city =", dict.city)

-- 测试4: 混合索引表
print("\n测试4: 混合索引表")
local mixed = {
    "第一个元素",
    "第二个元素",
    name = "测试表",
    [10] = "第十个位置"
}
print("  mixed[1] =", mixed[1])
print("  mixed[2] =", mixed[2])
print("  mixed.name =", mixed.name)
print("  mixed[10] =", mixed[10])

-- 测试5: 动态添加元素
print("\n测试5: 动态添加元素")
local dynamic = {}
dynamic[1] = "第一个"
dynamic["key"] = "值"
dynamic.field = "字段值"
print("  dynamic[1] =", dynamic[1])
print("  dynamic['key'] =", dynamic["key"])
print("  dynamic.field =", dynamic.field)

-- 测试6: 表元素修改
print("\n测试6: 表元素修改")
local modifiable = {a = 1, b = 2, c = 3}
print("  修改前: a=" .. modifiable.a .. ", b=" .. modifiable.b)
modifiable.a = 10
modifiable.b = 20
print("  修改后: a=" .. modifiable.a .. ", b=" .. modifiable.b)

-- 测试7: 表元素删除（设为nil）
print("\n测试7: 表元素删除")
local deletable = {x = 100, y = 200, z = 300}
print("  删除前: x=" .. tostring(deletable.x) .. ", y=" .. tostring(deletable.y))
deletable.x = nil
print("  删除后: x=" .. tostring(deletable.x) .. ", y=" .. tostring(deletable.y))

-- 测试8: 不存在的键访问
print("\n测试8: 不存在的键访问")
local testTable = {a = 1}
local nonExistent = testTable.nonExistent
print("  不存在的键值:", tostring(nonExistent))
if nonExistent == nil then
    print("✓ 不存在的键正确返回nil")
else
    print("✗ 不存在的键返回了非nil值")
end

-- 测试9: 数字键和字符串键
print("\n测试9: 数字键和字符串键")
local keyTest = {}
keyTest[1] = "数字键1"
keyTest["1"] = "字符串键1"
print("  keyTest[1] =", keyTest[1])
print("  keyTest['1'] =", keyTest["1"])

-- 测试10: 表的长度操作符
print("\n测试10: 表的长度操作符")
local lengthTest = {10, 20, 30, 40}
print("  #lengthTest =", #lengthTest)

local sparseArray = {[1] = "a", [3] = "c", [5] = "e"}
print("  #sparseArray =", #sparseArray)

-- 测试11: 嵌套表
print("\n测试11: 嵌套表")
local nested = {
    level1 = {
        level2 = {
            value = "深层值"
        }
    }
}
print("  nested.level1.level2.value =", nested.level1.level2.value)

-- 测试12: 表作为值
print("\n测试12: 表作为值")
local table1 = {a = 1}
local table2 = {b = 2}
local container = {
    first = table1,
    second = table2
}
print("  container.first.a =", container.first.a)
print("  container.second.b =", container.second.b)

print("\n=== 基础表操作测试完成 ===")
