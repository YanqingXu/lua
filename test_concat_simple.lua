-- 简单的__concat元方法测试
print("=== Simple __concat Metamethod Test ===")

-- 测试1: 基础字符串连接
print("\n--- Test 1: Basic string concatenation ---")
print("'Hello' .. ' World':", "Hello" .. " World")
print("'Number: ' .. 42:", "Number: " .. 42)

-- 测试2: 自定义__concat元方法
print("\n--- Test 2: Custom __concat metamethod ---")
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

print("About to test obj1 .. obj2...")
local result = obj1 .. obj2
print("obj1 .. obj2:", result)

-- 测试3: 字符串与自定义对象连接
print("\n--- Test 3: String with custom object concatenation ---")
local obj3 = {value = "Lua"}
setmetatable(obj3, concatMeta)

print("About to test 'Hello ' .. obj3...")
local result2 = "Hello " .. obj3
print("'Hello ' .. obj3:", result2)

print("\n=== Test completed ===")
