-- 修正的高级元方法测试
print("=== Advanced Metamethod Test (Fixed) ===")

-- Test 1: __index metamethod with table
print("Test 1: __index metamethod with table")
local t = {existing = "original"}
local fallback = {default_value = "from fallback table"}
local mt = {__index = fallback}
setmetatable(t, mt)

print("t.existing = " .. tostring(t.existing))
print("t.default_value = " .. tostring(t.default_value))
print("t.nonexistent = " .. tostring(t.nonexistent))

-- Test 2: __newindex metamethod with table
print("")
print("Test 2: __newindex metamethod with table")
local t2 = {existing = "original"}
local storage = {}
local mt2 = {__newindex = storage}
setmetatable(t2, mt2)

print("Setting t2.new_key = 'new_value'")
t2.new_key = "new_value"
print("t2.new_key = " .. tostring(t2.new_key))
print("storage.new_key = " .. tostring(storage.new_key))

-- Test 3: __tostring metamethod (function version)
print("")
print("Test 3: __tostring metamethod")
local t3 = {name = "test", value = 42}
local mt3 = {}

-- 设置 __tostring 为函数
mt3.__tostring = function(obj)
    return "Object{name=" .. tostring(obj.name) .. ", value=" .. tostring(obj.value) .. "}"
end

setmetatable(t3, mt3)

print("Created object with __tostring metamethod")
print("Object: " .. tostring(t3))

-- Test 4: 算术元方法结构测试
print("")
print("Test 4: Arithmetic metamethods structure")
local t4 = {value = 10}
local t5 = {value = 5}
local mt4 = {}

-- 设置 __add 元方法
mt4.__add = function(a, b)
    return {value = a.value + b.value}
end

setmetatable(t4, mt4)
setmetatable(t5, mt4)

print("t4.value = " .. tostring(t4.value))
print("t5.value = " .. tostring(t5.value))

-- 测试加法
local result = t4 + t5
print("(t4 + t5).value = " .. tostring(result.value))

-- Test 5: __call 元方法
print("")
print("Test 5: __call metamethod")
local t6 = {message = "Hello from callable object"}
local mt6 = {}

mt6.__call = function(obj, ...)
    print("Object called with args: " .. table.concat({...}, ", "))
    return obj.message
end

setmetatable(t6, mt6)

print("Created callable object")
local result = t6("arg1", "arg2")
print("Call result: " .. tostring(result))

-- Test 6: __concat 元方法
print("")
print("Test 6: __concat metamethod")
local t7 = {text = "Hello"}
local t8 = {text = " World"}
local mt7 = {}

mt7.__concat = function(a, b)
    return a.text .. b.text
end

setmetatable(t7, mt7)
setmetatable(t8, mt7)

print("t7.text = " .. t7.text)
print("t8.text = " .. t8.text)

local concat_result = t7 .. t8
print("t7 .. t8 = " .. concat_result)

print("")
print("=== Advanced test completed ===")
