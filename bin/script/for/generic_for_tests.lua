-- 泛型for循环测试
-- 测试pairs, ipairs等迭代器

print("=== 泛型for循环测试 ===")

-- 测试1: ipairs遍历数组
print("测试1: ipairs遍历数组")
local array = {10, 20, 30, 40, 50}
for i, v in ipairs(array) do
    print("  array[" .. i .. "] =", v)
end

-- 测试2: pairs遍历表
print("\n测试2: pairs遍历表")
local table1 = {
    name = "张三",
    age = 25,
    city = "北京"
}
for k, v in pairs(table1) do
    print("  " .. k .. " =", v)
end

-- 测试3: 混合索引表的遍历
print("\n测试3: 混合索引表的遍历")
local mixedTable = {
    "第一个",
    "第二个",
    name = "测试",
    [5] = "第五个"
}

print("使用ipairs:")
for i, v in ipairs(mixedTable) do
    print("  [" .. i .. "] =", v)
end

print("使用pairs:")
for k, v in pairs(mixedTable) do
    print("  [" .. tostring(k) .. "] =", v)
end

-- 测试4: 空表遍历
print("\n测试4: 空表遍历")
local emptyTable = {}
local count = 0
for k, v in pairs(emptyTable) do
    count = count + 1
    print("  这不应该被打印")
end
if count == 0 then
    print("✓ 空表遍历正确地没有执行")
else
    print("✗ 空表遍历错误地执行了")
end

-- 测试5: 嵌套表遍历
print("\n测试5: 嵌套表遍历")
local nestedTable = {
    group1 = {a = 1, b = 2},
    group2 = {c = 3, d = 4}
}
for groupName, group in pairs(nestedTable) do
    print("  组名:", groupName)
    for key, value in pairs(group) do
        print("    " .. key .. " =", value)
    end
end

-- 测试6: 数组中的nil值
print("\n测试6: 数组中的nil值")
local arrayWithNil = {1, 2, nil, 4, 5}
print("使用ipairs (遇到nil会停止):")
for i, v in ipairs(arrayWithNil) do
    print("  [" .. i .. "] =", v)
end

print("使用pairs (会跳过nil):")
for k, v in pairs(arrayWithNil) do
    print("  [" .. k .. "] =", v)
end

-- 测试7: 字符串键的表
print("\n测试7: 字符串键的表")
local stringKeyTable = {
    ["first name"] = "张",
    ["last name"] = "三",
    ["full name"] = "张三"
}
for k, v in pairs(stringKeyTable) do
    print("  '" .. k .. "' =", v)
end

-- 测试8: 数字键的表（非连续）
print("\n测试8: 数字键的表（非连续）")
local sparseArray = {
    [1] = "一",
    [3] = "三",
    [5] = "五",
    [10] = "十"
}

print("使用ipairs:")
for i, v in ipairs(sparseArray) do
    print("  [" .. i .. "] =", v)
end

print("使用pairs:")
for k, v in pairs(sparseArray) do
    print("  [" .. k .. "] =", v)
end

print("\n=== 泛型for循环测试完成 ===")
