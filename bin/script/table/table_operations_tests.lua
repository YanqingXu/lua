-- 表操作高级测试
-- 测试表的高级操作和边界情况

print("=== 表操作高级测试 ===")

-- 测试1: 表的复制（浅拷贝）
print("测试1: 表的浅拷贝")
local original = {a = 1, b = 2, c = {nested = 3}}
local copy = {}
for k, v in pairs(original) do
    copy[k] = v
end
print("  original.a =", original.a)
print("  copy.a =", copy.a)
copy.a = 10
print("  修改copy后:")
print("  original.a =", original.a)
print("  copy.a =", copy.a)

-- 测试2: 表的比较
print("\n测试2: 表的比较")
local table1 = {a = 1, b = 2}
local table2 = {a = 1, b = 2}
local table3 = table1

print("  table1 == table2:", table1 == table2)  -- false，不同的表对象
print("  table1 == table3:", table1 == table3)  -- true，同一个表对象

-- 测试3: 表作为键
print("\n测试3: 表作为键")
local keyTable = {x = 1}
local valueTable = {y = 2}
local tableWithTableKey = {}
tableWithTableKey[keyTable] = valueTable
print("  使用表作为键设置成功")
print("  tableWithTableKey[keyTable].y =", tableWithTableKey[keyTable].y)

-- 测试4: 函数作为表值
print("\n测试4: 函数作为表值")
local functionTable = {
    add = function(a, b) return a + b end,
    multiply = function(a, b) return a * b end
}
print("  functionTable.add(3, 4) =", functionTable.add(3, 4))
print("  functionTable.multiply(3, 4) =", functionTable.multiply(3, 4))

-- 测试5: 表的动态扩展
print("\n测试5: 表的动态扩展")
local expandable = {}
for i = 1, 5 do
    expandable[i] = i * i
end
print("  动态添加后的表:")
for i = 1, 5 do
    print("    expandable[" .. i .. "] =", expandable[i])
end

-- 测试6: 稀疏数组
print("\n测试6: 稀疏数组")
local sparse = {}
sparse[1] = "第一个"
sparse[100] = "第一百个"
sparse[1000] = "第一千个"
print("  sparse[1] =", sparse[1])
print("  sparse[100] =", sparse[100])
print("  sparse[1000] =", sparse[1000])
print("  #sparse =", #sparse)

-- 测试7: 表的序列部分和哈希部分
print("\n测试7: 表的序列部分和哈希部分")
local hybrid = {
    "序列1",  -- [1]
    "序列2",  -- [2]
    "序列3",  -- [3]
    name = "哈希键",
    [10] = "跳跃索引",
    age = 25
}
print("  序列部分长度: #hybrid =", #hybrid)
print("  hybrid[1] =", hybrid[1])
print("  hybrid.name =", hybrid.name)
print("  hybrid[10] =", hybrid[10])

-- 测试8: 表的遍历顺序
print("\n测试8: 表的遍历顺序")
local orderTest = {
    c = 3,
    a = 1,
    b = 2,
    [1] = "数字1",
    [2] = "数字2"
}
print("  pairs遍历顺序:")
for k, v in pairs(orderTest) do
    print("    " .. tostring(k) .. " =", v)
end

-- 测试9: 表的大小估算
print("\n测试9: 表的大小估算")
local sizeTest = {}
local count = 0
-- 添加一些元素
for i = 1, 10 do
    sizeTest[i] = i
    count = count + 1
end
sizeTest.name = "测试"
count = count + 1
sizeTest.value = 100
count = count + 1

print("  手动计数的元素数量:", count)
local pairsCount = 0
for k, v in pairs(sizeTest) do
    pairsCount = pairsCount + 1
end
print("  pairs遍历的元素数量:", pairsCount)

-- 测试10: 表的nil值处理
print("\n测试10: 表的nil值处理")
local nilTest = {a = 1, b = 2, c = 3}
print("  设置前: nilTest.b =", nilTest.b)
nilTest.b = nil
print("  设置为nil后: nilTest.b =", tostring(nilTest.b))

-- 验证nil值确实被删除
local foundB = false
for k, v in pairs(nilTest) do
    if k == "b" then
        foundB = true
    end
end
if not foundB then
    print("✓ nil值正确地从表中删除")
else
    print("✗ nil值没有从表中删除")
end

print("\n=== 表操作高级测试完成 ===")
