-- 数学运算测试
-- 测试数学库的各种功能

print("=== 数学运算测试 ===")

-- 测试1: 基本算术运算
print("测试1: 基本算术运算")
local a = 10
local b = 3
print("  a =", a, ", b =", b)
print("  a + b =", a + b)
print("  a - b =", a - b)
print("  a * b =", a * b)
print("  a / b =", a / b)
print("  a % b =", a % b)
print("  a ^ b =", a ^ b)

-- 测试2: 一元运算
print("\n测试2: 一元运算")
local x = 5
print("  x =", x)
print("  -x =", -x)

-- 测试3: math.abs函数
print("\n测试3: math.abs函数")
if math and math.abs then
    print("  math.abs(-5) =", math.abs(-5))
    print("  math.abs(5) =", math.abs(5))
    print("  math.abs(0) =", math.abs(0))
else
    print("  math.abs函数不可用")
end

-- 测试4: math.max和math.min
print("\n测试4: math.max和math.min")
if math and math.max and math.min then
    print("  math.max(1, 5, 3) =", math.max(1, 5, 3))
    print("  math.min(1, 5, 3) =", math.min(1, 5, 3))
    print("  math.max(-1, -5, -3) =", math.max(-1, -5, -3))
else
    print("  math.max/min函数不可用")
end

-- 测试5: math.floor和math.ceil
print("\n测试5: math.floor和math.ceil")
if math and math.floor and math.ceil then
    local num = 3.7
    print("  num =", num)
    print("  math.floor(num) =", math.floor(num))
    print("  math.ceil(num) =", math.ceil(num))
    
    local negNum = -3.7
    print("  negNum =", negNum)
    print("  math.floor(negNum) =", math.floor(negNum))
    print("  math.ceil(negNum) =", math.ceil(negNum))
else
    print("  math.floor/ceil函数不可用")
end

-- 测试6: math.sqrt
print("\n测试6: math.sqrt")
if math and math.sqrt then
    print("  math.sqrt(16) =", math.sqrt(16))
    print("  math.sqrt(2) =", math.sqrt(2))
    print("  math.sqrt(0) =", math.sqrt(0))
else
    print("  math.sqrt函数不可用")
end

-- 测试7: 三角函数
print("\n测试7: 三角函数")
if math and math.sin and math.cos and math.tan then
    local angle = math.pi / 4  -- 45度
    print("  angle = π/4")
    print("  math.sin(angle) =", math.sin(angle))
    print("  math.cos(angle) =", math.cos(angle))
    print("  math.tan(angle) =", math.tan(angle))
else
    print("  三角函数不可用")
end

-- 测试8: math.pi常量
print("\n测试8: math.pi常量")
if math and math.pi then
    print("  math.pi =", math.pi)
    print("  2 * math.pi =", 2 * math.pi)
else
    print("  math.pi常量不可用")
end

-- 测试9: math.exp和math.log
print("\n测试9: math.exp和math.log")
if math and math.exp and math.log then
    print("  math.exp(1) =", math.exp(1))  -- e
    print("  math.log(math.exp(1)) =", math.log(math.exp(1)))  -- 应该是1
    print("  math.log(10) =", math.log(10))
else
    print("  math.exp/log函数不可用")
end

-- 测试10: math.random
print("\n测试10: math.random")
if math and math.random then
    print("  math.random() =", math.random())
    print("  math.random(10) =", math.random(10))
    print("  math.random(5, 15) =", math.random(5, 15))
    
    -- 设置随机种子
    if math.randomseed then
        math.randomseed(12345)
        print("  设置种子后 math.random() =", math.random())
    end
else
    print("  math.random函数不可用")
end

-- 测试11: math.deg和math.rad
print("\n测试11: math.deg和math.rad")
if math and math.deg and math.rad then
    print("  math.deg(math.pi) =", math.deg(math.pi))  -- 180度
    print("  math.rad(180) =", math.rad(180))  -- π弧度
else
    print("  math.deg/rad函数不可用")
end

-- 测试12: math.fmod
print("\n测试12: math.fmod")
if math and math.fmod then
    print("  math.fmod(10, 3) =", math.fmod(10, 3))
    print("  math.fmod(-10, 3) =", math.fmod(-10, 3))
    print("  10 % 3 =", 10 % 3)
    print("  -10 % 3 =", -10 % 3)
else
    print("  math.fmod函数不可用")
end

-- 测试13: 数值精度测试
print("\n测试13: 数值精度测试")
local small = 0.1 + 0.2
print("  0.1 + 0.2 =", small)
print("  0.1 + 0.2 == 0.3:", small == 0.3)

-- 测试14: 大数运算
print("\n测试14: 大数运算")
local big1 = 1e15
local big2 = 2e15
print("  1e15 + 2e15 =", big1 + big2)
print("  1e15 * 2 =", big1 * 2)

print("\n=== 数学运算测试完成 ===")
