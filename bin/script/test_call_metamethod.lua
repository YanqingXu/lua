-- 测试__call元方法功能
print("=== 测试__call元方法功能 ===")

-- 测试1: 基本的__call元方法
print("\n1. 基本__call元方法测试:")
local mt1 = {
    __call = function(t, ...)
        print("__call被调用，参数:", ...)
        return "call_result"
    end
}

local obj1 = setmetatable({name = "test_obj"}, mt1)
print("obj1创建完成，类型:", type(obj1))
print("obj1的元表:", getmetatable(obj1))

-- 尝试调用obj1
print("尝试调用obj1():")
local success, result = pcall(function()
    return obj1()
end)
print("调用成功:", success)
if success then
    print("调用结果:", result)
else
    print("调用错误:", result)
end

-- 测试2: 带参数的__call
print("\n2. 带参数的__call测试:")
local mt2 = {
    __call = function(t, a, b)
        print("__call被调用，参数a:", a, "参数b:", b)
        return a + b
    end
}

local obj2 = setmetatable({}, mt2)
print("尝试调用obj2(10, 20):")
local success2, result2 = pcall(function()
    return obj2(10, 20)
end)
print("调用成功:", success2)
if success2 then
    print("调用结果:", result2)
else
    print("调用错误:", result2)
end

-- 测试3: __call返回多个值
print("\n3. __call返回多个值测试:")
local mt3 = {
    __call = function(t, x)
        return x, x * 2, x * 3
    end
}

local obj3 = setmetatable({}, mt3)
print("尝试调用obj3(5):")
local success3, a, b, c = pcall(function()
    return obj3(5)
end)
print("调用成功:", success3)
if success3 then
    print("返回值:", a, b, c)
else
    print("调用错误:", a)
end

-- 测试4: 检查元方法是否存在
print("\n4. 元方法存在性检查:")
local mt4 = {
    __call = function(t) return "exists" end
}
local obj4 = setmetatable({}, mt4)

-- 手动检查元方法
local metatable = getmetatable(obj4)
if metatable then
    print("元表存在")
    print("__call元方法:", metatable.__call)
    print("__call类型:", type(metatable.__call))
else
    print("元表不存在")
end

-- 测试5: 直接调用元方法
print("\n5. 直接调用元方法测试:")
if metatable and metatable.__call then
    print("直接调用元方法:")
    local direct_result = metatable.__call(obj4)
    print("直接调用结果:", direct_result)
end

print("\n=== 测试完成 ===")
