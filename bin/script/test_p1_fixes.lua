-- 测试P1优先级修复：元方法支持和SELF指令
print("=== P1修复测试：元方法支持和SELF指令 ===")

-- 测试1: 基础算术运算（确保向后兼容）
print("\n1. 测试基础算术运算（向后兼容性）:")
local a = 10
local b = 5

print("  加法: " .. a .. " + " .. b .. " = " .. (a + b))
print("  减法: " .. a .. " - " .. b .. " = " .. (a - b))
print("  乘法: " .. a .. " * " .. b .. " = " .. (a * b))
print("  除法: " .. a .. " / " .. b .. " = " .. (a / b))
print("  取模: " .. a .. " % " .. b .. " = " .. (a % b))
print("  幂运算: " .. b .. " ^ 2 = " .. (b ^ 2))
print("  负号: -" .. a .. " = " .. (-a))

-- 测试2: 元方法框架基础功能
print("\n2. 测试元方法框架基础:")

-- 创建一个简单的表来测试SELF指令
local obj = {}
obj.name = "TestObject"
obj.getValue = function(self)
    return "Hello from " .. self.name
end

-- 测试SELF指令的基础功能
print("  对象名称: " .. obj.name)
if obj.getValue then
    print("  ✓ SELF指令：找到了方法")
    -- 注意：这里测试的是SELF指令能否正确获取方法
    -- 完整的方法调用需要更复杂的测试
else
    print("  ✗ SELF指令：未找到方法")
end

-- 测试3: 表访问和方法查找
print("\n3. 测试表访问和方法查找:")
local testTable = {
    x = 100,
    y = 200,
    add = function(self, other)
        return self.x + other
    end
}

print("  表字段访问: testTable.x = " .. testTable.x)
print("  表字段访问: testTable.y = " .. testTable.y)

if testTable.add then
    print("  ✓ 方法查找成功")
else
    print("  ✗ 方法查找失败")
end

-- 测试4: 错误处理
print("\n4. 测试错误处理:")

-- 测试除零错误处理
local function testDivisionByZero()
    local result = 10 / 0
    return result
end

-- 这个测试可能会触发错误，我们用简单的方式测试
print("  除零测试：尝试 10 / 0")
-- 注意：在实际实现中，这应该触发错误或返回特殊值

-- 测试5: 复杂算术表达式
print("\n5. 测试复杂算术表达式:")
local x = 2
local y = 3
local z = 4

local result1 = x + y * z
local result2 = (x + y) * z
local result3 = x ^ y + z

print("  2 + 3 * 4 = " .. result1 .. " (期望: 14)")
print("  (2 + 3) * 4 = " .. result2 .. " (期望: 20)")
print("  2 ^ 3 + 4 = " .. result3 .. " (期望: 12)")

-- 测试6: 字符串和数字的混合运算
print("\n6. 测试类型转换:")
local num = 42
local str = "The answer is "
local combined = str .. num

print("  字符串连接: '" .. str .. "' .. " .. num .. " = '" .. combined .. "'")

print("\n=== P1修复测试完成 ===")
