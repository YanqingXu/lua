-- 逻辑表达式测试
-- 测试复杂的逻辑运算符组合

print("=== 逻辑表达式测试 ===")

-- 测试1: and运算符
print("测试1: and运算符")
local a = true
local b = true
local c = false

if a and b then
    print("✓ true and true = true")
else
    print("✗ true and true = false")
end

if a and c then
    print("✗ true and false = true")
else
    print("✓ true and false = false")
end

-- 测试2: or运算符
print("\n测试2: or运算符")
if a or c then
    print("✓ true or false = true")
else
    print("✗ true or false = false")
end

if c or false then
    print("✗ false or false = true")
else
    print("✓ false or false = false")
end

-- 测试3: not运算符
print("\n测试3: not运算符")
if not c then
    print("✓ not false = true")
else
    print("✗ not false = false")
end

if not a then
    print("✗ not true = true")
else
    print("✓ not true = false")
end

-- 测试4: 复合逻辑表达式
print("\n测试4: 复合逻辑表达式")
local x = 5
local y = 10
local z = 15

if x < y and y < z then
    print("✓ x < y < z")
else
    print("✗ 条件不成立")
end

if x > y or y < z then
    print("✓ x > y 或 y < z")
else
    print("✗ 两个条件都不成立")
end

-- 测试5: 短路求值
print("\n测试5: 短路求值测试")
local function returnTrue()
    print("  returnTrue被调用")
    return true
end

local function returnFalse()
    print("  returnFalse被调用")
    return false
end

print("测试 false and returnTrue():")
if false and returnTrue() then
    print("✗ 结果为真")
else
    print("✓ 结果为假（returnTrue不应被调用）")
end

print("测试 true or returnFalse():")
if true or returnFalse() then
    print("✓ 结果为真（returnFalse不应被调用）")
else
    print("✗ 结果为假")
end

-- 测试6: 比较运算符组合
print("\n测试6: 比较运算符组合")
local num1 = 10
local num2 = 20
local num3 = 10

if num1 == num3 and num1 < num2 then
    print("✓ num1 == num3 且 num1 < num2")
else
    print("✗ 条件不成立")
end

if num1 ~= num2 and num2 > num3 then
    print("✓ num1 ~= num2 且 num2 > num3")
else
    print("✗ 条件不成立")
end

-- 测试7: 复杂的括号组合
print("\n测试7: 复杂的括号组合")
local p = true
local q = false
local r = true

if (p and q) or (not q and r) then
    print("✓ (p and q) or (not q and r)")
else
    print("✗ 条件不成立")
end

if not (p and q) and (p or r) then
    print("✓ not (p and q) and (p or r)")
else
    print("✗ 条件不成立")
end

print("\n=== 逻辑表达式测试完成 ===")
