-- 简化的元方法测试，用于调试问题
print("=== Complete Metamethod Test (Debug) ===")

-- 测试1: 基本的 __index 和 __newindex
print("Test 1: Basic __index and __newindex")
local obj = {}
local storage = {}

print("Creating metatable...")
local meta = {}

-- 先测试不带函数的版本
meta.__index = storage
meta.__newindex = storage

print("Setting metatable...")
setmetatable(obj, meta)

print("Testing __newindex...")
obj.test = "hello"
print("obj.test = " .. tostring(obj.test))
print("storage.test = " .. tostring(storage.test))

print("Test 1 completed")

-- 测试2: __call（如果支持）
print("\nTest 2: __call")
local callable = {}
local call_meta = {}

-- 检查是否支持函数形式的元方法
if type(function() end) == "function" then
    print("Functions are supported, testing __call")
    call_meta.__call = function(t, ...)
        print("__call invoked")
        return "called"
    end
    
    setmetatable(callable, call_meta)
    
    -- 尝试调用
    pcall(function()
        local result = callable("arg1")
        print("Call result: " .. tostring(result))
    end)
else
    print("Functions not fully supported, skipping __call test")
end

print("\nTest 2 completed")

-- 测试3: 简单的 __concat
print("\nTest 3: Simple __concat")
local str_obj = {value = "Hello"}
local str_obj2 = {value = "World"}

-- 先不用元方法，直接测试字符串连接
local simple_concat = str_obj.value .. " " .. str_obj2.value
print("Simple concat: " .. simple_concat)

-- 如果基本连接工作，再尝试元方法
local concat_meta = {}
concat_meta.__concat = function(a, b)
    print("__concat called")
    return a.value .. " " .. b.value
end

setmetatable(str_obj, concat_meta)
setmetatable(str_obj2, concat_meta)

-- 小心测试，可能会失败
local success, result = pcall(function()
    return str_obj .. str_obj2
end)

if success then
    print("Concat result: " .. tostring(result))
else
    print("Concat failed: " .. tostring(result))
end

print("\nTest 3 completed")

-- 测试4: 算术元方法
print("\nTest 4: Arithmetic metamethods")
local num_obj = {value = 10}
local num_obj2 = {value = 5}

local arith_meta = {}
arith_meta.__add = function(a, b)
    print("__add called")
    return {value = a.value + b.value}
end

setmetatable(num_obj, arith_meta)
setmetatable(num_obj2, arith_meta)

-- 小心测试算术
local success2, add_result = pcall(function()
    return num_obj + num_obj2
end)

if success2 then
    print("Add result: " .. tostring(add_result.value))
else
    print("Add failed: " .. tostring(add_result))
end

print("\nTest 4 completed")

print("\n=== Debug test completed ===")
