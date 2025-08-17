-- 嵌套if语句测试
-- 测试复杂的嵌套条件结构

print("=== 嵌套if语句测试 ===")

-- 测试1: 简单嵌套
print("测试1: 简单嵌套")
local a = 10
local b = 5
if a > 5 then
    print("✓ a大于5")
    if b > 3 then
        print("✓ b也大于3")
    else
        print("✗ b不大于3")
    end
else
    print("✗ a不大于5")
end

-- 测试2: 深层嵌套
print("\n测试2: 深层嵌套")
local level1 = true
local level2 = true
local level3 = false
if level1 then
    print("✓ 进入第1层")
    if level2 then
        print("✓ 进入第2层")
        if level3 then
            print("✗ 进入第3层")
        else
            print("✓ 第3层条件为假")
        end
    else
        print("✗ 第2层条件为假")
    end
else
    print("✗ 第1层条件为假")
end

-- 测试3: 复杂条件嵌套
print("\n测试3: 复杂条件嵌套")
local age = 25
local hasLicense = true
local hasInsurance = true

if age >= 18 then
    print("✓ 年龄符合要求")
    if hasLicense then
        print("✓ 有驾照")
        if hasInsurance then
            print("✓ 有保险，可以开车")
        else
            print("✗ 没有保险，不能开车")
        end
    else
        print("✗ 没有驾照，不能开车")
    end
else
    print("✗ 年龄不符合要求")
end

-- 测试4: 嵌套中的elseif
print("\n测试4: 嵌套中的elseif")
local weather = "sunny"
local temperature = 25

if weather == "sunny" then
    print("✓ 天气晴朗")
    if temperature > 30 then
        print("✗ 太热了")
    elseif temperature > 20 then
        print("✓ 温度适宜")
    else
        print("✗ 有点冷")
    end
elseif weather == "rainy" then
    print("✗ 下雨天")
else
    print("✗ 其他天气")
end

-- 测试5: 多重嵌套的else分支
print("\n测试5: 多重嵌套的else分支")
local x = 3
local y = 7

if x > 5 then
    print("✗ x大于5")
    if y > 10 then
        print("✗ y大于10")
    else
        print("✗ y不大于10")
    end
else
    print("✓ x不大于5")
    if y > 5 then
        print("✓ y大于5")
    else
        print("✗ y不大于5")
    end
end

print("\n=== 嵌套if语句测试完成 ===")
