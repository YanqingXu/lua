-- 测试__concat元方法
print("=== __concat Metamethod Test ===")

-- 测试1: 基础字符串连接
print("\n--- Test 1: Basic string concatenation ---")
print("'Hello' .. ' World':", "Hello" .. " World")
print("'Number: ' .. 42:", "Number: " .. 42)

-- 测试2: 表的默认连接（应该失败）
print("\n--- Test 2: Table concatenation without metamethod ---")
local table1 = {value = "Hello"}
local table2 = {value = "World"}

local success1, result1 = pcall(function()
    return table1 .. table2
end)

if success1 then
    print("table1 .. table2:", result1)
else
    print("table1 .. table2 failed (expected):", result1)
end

-- 测试3: 自定义__concat元方法
print("\n--- Test 3: Custom __concat metamethod ---")
local obj1 = {value = "Hello"}
local obj2 = {value = "World"}

-- 创建共享的元表
local concatMeta = {
    __concat = function(a, b)
        print("__concat called: a.value =", a.value, "b.value =", b.value)
        return a.value .. " " .. b.value
    end
}

setmetatable(obj1, concatMeta)
setmetatable(obj2, concatMeta)

local success2, result2 = pcall(function()
    return obj1 .. obj2
end)

if success2 then
    print("obj1 .. obj2:", result2)
else
    print("obj1 .. obj2 failed:", result2)
end

-- 测试4: 复杂的__concat元方法
print("\n--- Test 4: Complex __concat metamethod ---")
local person1 = {name = "Alice", age = 30}
local person2 = {name = "Bob", age = 25}

local personMeta = {
    __concat = function(a, b)
        print("Person __concat called")
        return a.name .. " and " .. b.name .. " (ages " .. a.age .. " and " .. b.age .. ")"
    end
}

setmetatable(person1, personMeta)
setmetatable(person2, personMeta)

local success3, result3 = pcall(function()
    return person1 .. person2
end)

if success3 then
    print("person1 .. person2:", result3)
else
    print("person1 .. person2 failed:", result3)
end

print("\n=== Test completed ===")
