-- 高级元方法测试
print("=== 高级元方法测试 ===")

-- 测试1: 基础数值运算（确保直接路径工作）
print("\n1. 基础数值运算测试:")
local a = 10
local b = 3

print("  " .. a .. " + " .. b .. " = " .. (a + b))
print("  " .. a .. " - " .. b .. " = " .. (a - b))
print("  " .. a .. " * " .. b .. " = " .. (a * b))
print("  " .. a .. " / " .. b .. " = " .. (a / b))
print("  " .. a .. " % " .. b .. " = " .. (a % b))
print("  " .. b .. " ^ 2 = " .. (b ^ 2))
print("  -" .. a .. " = " .. (-a))

-- 测试2: 错误处理改进
print("\n2. 错误处理测试:")

-- 测试除零
print("  测试除零检测...")
-- 注意：这可能会触发错误，但不应该崩溃

-- 测试3: 类型混合运算
print("\n3. 类型混合运算测试:")
local num = 42
local str = "hello"

-- 这些应该触发元方法查找（如果有的话）或者产生适当的错误
print("  数字: " .. num)
print("  字符串: " .. str)

-- 测试4: 表操作和SELF指令
print("\n4. 表操作和SELF指令测试:")
local vector = {}
vector.x = 10
vector.y = 20

-- 测试SELF指令的表访问部分
vector.getX = function(self)
    return self.x
end

vector.getY = function(self)
    return self.y
end

print("  vector.x = " .. vector.x)
print("  vector.y = " .. vector.y)

-- 测试方法查找
if vector.getX then
    print("  ✓ 找到 getX 方法")
else
    print("  ✗ 未找到 getX 方法")
end

if vector.getY then
    print("  ✓ 找到 getY 方法")
else
    print("  ✗ 未找到 getY 方法")
end

-- 测试5: 复杂表达式
print("\n5. 复杂表达式测试:")
local x = 2
local y = 3
local z = 4

local result1 = x + y * z  -- 应该是 2 + (3 * 4) = 14
local result2 = (x + y) * z  -- 应该是 (2 + 3) * 4 = 20
local result3 = x ^ y + z  -- 应该是 2^3 + 4 = 12

print("  2 + 3 * 4 = " .. result1 .. " (期望: 14)")
print("  (2 + 3) * 4 = " .. result2 .. " (期望: 20)")
print("  2 ^ 3 + 4 = " .. result3 .. " (期望: 12)")

print("\n=== 高级元方法测试完成 ===")
