-- 比较元方法测试
print("=== Comparison Metamethod Test ===")

-- 测试1: __eq 元方法
print("Test 1: __eq metamethod")
local obj1 = {value = 10}
local obj2 = {value = 10}
local obj3 = {value = 20}

local meta = {
    __eq = function(a, b)
        print("__eq called: " .. tostring(a.value) .. " == " .. tostring(b.value))
        return a.value == b.value
    end
}

setmetatable(obj1, meta)
setmetatable(obj2, meta)
setmetatable(obj3, meta)

print("Testing obj1 == obj2 (both have value 10):")
local result1 = obj1 == obj2
print("Result: " .. tostring(result1))

print("Testing obj1 == obj3 (10 vs 20):")
local result2 = obj1 == obj3
print("Result: " .. tostring(result2))

-- 测试2: __lt 元方法
print("\nTest 2: __lt metamethod")
local obj4 = {value = 5}
local obj5 = {value = 15}

local meta2 = {
    __lt = function(a, b)
        print("__lt called: " .. tostring(a.value) .. " < " .. tostring(b.value))
        return a.value < b.value
    end
}

setmetatable(obj4, meta2)
setmetatable(obj5, meta2)

print("Testing obj4 < obj5 (5 < 15):")
local result3 = obj4 < obj5
print("Result: " .. tostring(result3))

print("Testing obj5 < obj4 (15 < 5):")
local result4 = obj5 < obj4
print("Result: " .. tostring(result4))

-- 测试3: __le 元方法
print("\nTest 3: __le metamethod")
local obj6 = {value = 8}
local obj7 = {value = 8}
local obj8 = {value = 12}

local meta3 = {
    __le = function(a, b)
        print("__le called: " .. tostring(a.value) .. " <= " .. tostring(b.value))
        return a.value <= b.value
    end
}

setmetatable(obj6, meta3)
setmetatable(obj7, meta3)
setmetatable(obj8, meta3)

print("Testing obj6 <= obj7 (8 <= 8):")
local result5 = obj6 <= obj7
print("Result: " .. tostring(result5))

print("Testing obj6 <= obj8 (8 <= 12):")
local result6 = obj6 <= obj8
print("Result: " .. tostring(result6))

print("Testing obj8 <= obj6 (12 <= 8):")
local result7 = obj8 <= obj6
print("Result: " .. tostring(result7))

print("\n=== Comparison metamethod test completed ===")
