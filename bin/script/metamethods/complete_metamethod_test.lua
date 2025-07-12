-- 元方法完整功能测试
print("=== Complete Metamethod Test ===")

-- 测试1: __index 和 __newindex
print("Test 1: __index and __newindex")
local obj = {}
local storage = {}
local meta = {
    __index = function(t, k)
        print("__index called for: " .. tostring(k))
        return storage[k]
    end,
    __newindex = function(t, k, v)
        print("__newindex called for: " .. tostring(k) .. " = " .. tostring(v))
        storage[k] = v
    end
}

setmetatable(obj, meta)
obj.test = "hello"
print("obj.test = " .. tostring(obj.test))

-- 测试2: __call
print("\nTest 2: __call")
local callable = {}
local call_meta = {
    __call = function(t, ...)
        print("__call invoked with args: " .. tostring(...))
        return "called"
    end
}

setmetatable(callable, call_meta)
local result = callable("arg1", "arg2")
print("Call result: " .. tostring(result))

-- 测试3: __concat
print("\nTest 3: __concat")
local str_obj = {value = "Hello"}
local concat_meta = {
    __concat = function(a, b)
        print("__concat called")
        return tostring(a.value) .. " " .. tostring(b.value)
    end
}

setmetatable(str_obj, concat_meta)
local str_obj2 = {value = "World"}
setmetatable(str_obj2, concat_meta)
local concat_result = str_obj .. str_obj2
print("Concat result: " .. tostring(concat_result))

-- 测试4: 算数元方法
print("\nTest 4: Arithmetic metamethods")
local num_obj = {value = 10}
local arith_meta = {
    __add = function(a, b)
        print("__add called")
        return {value = a.value + b.value}
    end,
    __sub = function(a, b)
        print("__sub called")
        return {value = a.value - b.value}
    end
}

setmetatable(num_obj, arith_meta)
local num_obj2 = {value = 5}
setmetatable(num_obj2, arith_meta)
local add_result = num_obj + num_obj2
print("Add result: " .. tostring(add_result.value))

print("\n=== Complete metamethod test completed ===")
