-- 调试__concat元方法
print("=== Debug __concat Metamethod ===")

-- 测试1: 基础字符串连接（应该工作）
print("\n--- Test 1: Basic string concatenation ---")
local result1 = "Hello" .. " World"
print("Result:", result1)

-- 测试2: 创建带有__concat元方法的对象
print("\n--- Test 2: Create object with __concat metamethod ---")
local obj = {value = "Lua"}
print("obj created, value:", obj.value)

local meta = {
    __concat = function(a, b)
        print("__concat metamethod called!")
        print("a type:", type(a), "b type:", type(b))
        if type(a) == "table" and a.value then
            print("a.value:", a.value)
        end
        if type(b) == "table" and b.value then
            print("b.value:", b.value)
        end
        
        local aStr = (type(a) == "table" and a.value) or tostring(a)
        local bStr = (type(b) == "table" and b.value) or tostring(b)
        
        return aStr .. " " .. bStr
    end
}

setmetatable(obj, meta)
print("Metatable set successfully")

-- 测试3: 两个对象连接（应该工作）
print("\n--- Test 3: Object to object concatenation ---")
local obj2 = {value = "Programming"}
setmetatable(obj2, meta)

local result2 = obj .. obj2
print("obj .. obj2:", result2)

print("\n=== Test completed ===")
