-- 修复后的综合元方法测试
print("=== Fixed Comprehensive Metamethod Test ===")

-- 测试1: 基本元表设置
print("Test 1: Basic metatable setup")
local obj = {}
local meta = {
    __index = function(t, k)
        print("__index called for key: " .. tostring(k))
        return "default_" .. tostring(k)
    end,
    __newindex = function(t, k, v)
        print("__newindex called for key: " .. tostring(k) .. " = " .. tostring(v))
        rawset(t, k, v)
    end
}

setmetatable(obj, meta)
print("Metatable set successfully")

-- 测试2: 测试__index
print("\nTest 2: Testing __index")
local value = obj.test_key
print("Retrieved value: " .. tostring(value))

-- 测试3: 测试__newindex
print("\nTest 3: Testing __newindex")
obj.new_key = "new_value"
print("Assignment completed")

-- 测试4: 表字面量替代测试
print("\nTest 4: Table literal alternative")
local t = {}
t.x = 10
t.y = 20
print("t.x = " .. tostring(t.x))
print("t.y = " .. tostring(t.y))

-- 测试5: 字符串连接测试
print("\nTest 5: String concatenation")
local s1 = "Hello"
local s2 = "World"
local result = s1 .. " " .. s2
print("Concatenation result: " .. result)

print("\n=== Fixed comprehensive test completed ===")
