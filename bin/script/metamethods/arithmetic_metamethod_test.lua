-- 算术元方法测试
print("=== Arithmetic Metamethod Test ===")

-- 测试1: __add 元方法
print("Test 1: __add metamethod")
local obj1 = {value = 10}
local obj2 = {value = 20}

local meta = {
    __add = function(a, b)
        print("__add called: " .. tostring(a.value) .. " + " .. tostring(b.value))
        return {value = a.value + b.value}
    end
}

setmetatable(obj1, meta)
setmetatable(obj2, meta)

print("Testing obj1 + obj2:")
local result = obj1 + obj2
print("Result value: " .. tostring(result.value))

-- 测试2: __sub 元方法
print("\nTest 2: __sub metamethod")
local obj3 = {value = 30}
local obj4 = {value = 10}

local meta2 = {
    __sub = function(a, b)
        print("__sub called: " .. tostring(a.value) .. " - " .. tostring(b.value))
        return {value = a.value - b.value}
    end
}

setmetatable(obj3, meta2)
setmetatable(obj4, meta2)

print("Testing obj3 - obj4:")
local result2 = obj3 - obj4
print("Result value: " .. tostring(result2.value))

-- 测试3: __mul 元方法
print("\nTest 3: __mul metamethod")
local obj5 = {value = 5}
local obj6 = {value = 6}

local meta3 = {
    __mul = function(a, b)
        print("__mul called: " .. tostring(a.value) .. " * " .. tostring(b.value))
        return {value = a.value * b.value}
    end
}

setmetatable(obj5, meta3)
setmetatable(obj6, meta3)

print("Testing obj5 * obj6:")
local result3 = obj5 * obj6
print("Result value: " .. tostring(result3.value))

-- 测试4: __unm 元方法（一元负号）
print("\nTest 4: __unm metamethod")
local obj7 = {value = 42}

local meta4 = {
    __unm = function(a)
        print("__unm called: -" .. tostring(a.value))
        return {value = -a.value}
    end
}

setmetatable(obj7, meta4)

print("Testing -obj7:")
local result4 = -obj7
print("Result value: " .. tostring(result4.value))

print("\n=== Arithmetic metamethod test completed ===")
