-- 简单的修复验证测试
print("=== 简单修复验证 ===")

-- 测试比较指令
print("1. 比较指令测试:")
if 5 == 5 then
    print("  EQ: 5 == 5 正确")
end

if 3 < 7 then
    print("  LT: 3 < 7 正确")
end

if 4 <= 4 then
    print("  LE: 4 <= 4 正确")
end

-- 测试字符串连接
print("2. 字符串连接测试:")
local hello = "Hello"
local world = "World"
local exclaim = "!"
local result = hello .. world .. exclaim
print("  连接结果: " .. result)

-- 测试简单循环
print("3. 循环测试:")
for i = 1, 3 do
    if i <= 3 then
        print("  循环 " .. i .. " 正确")
    end
end

print("=== 验证完成 ===")
