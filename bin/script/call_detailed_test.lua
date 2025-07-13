-- 详细测试__call元方法
print("=== 详细__call测试 ===")

-- 测试1: 基本调用
print("\n1. 基本调用测试:")
local mt1 = {
    __call = function(t, ...)
        print("参数个数:", select("#", ...))
        print("参数:", ...)
        return "基本调用成功"
    end
}

local obj1 = setmetatable({value = 100}, mt1)
local result1 = obj1("hello", "world", 123)
print("返回值:", result1)

-- 测试2: 带返回值的调用
print("\n2. 多返回值测试:")
local mt2 = {
    __call = function(t, x)
        return x, x * 2, x * 3
    end
}

local obj2 = setmetatable({}, mt2)
local a, b, c = obj2(5)
print("返回值:", a, b, c)

-- 测试3: 访问self
print("\n3. 访问self测试:")
local mt3 = {
    __call = function(self, name)
        return "Hello " .. name .. ", my value is " .. (self.data or "nil")
    end
}

local obj3 = setmetatable({data = "test_data"}, mt3)
local result3 = obj3("Alice")
print("结果:", result3)

-- 测试4: 错误处理
print("\n4. 错误处理测试:")
local mt4 = {
    __call = function(t)
        error("故意的错误")
    end
}

local obj4 = setmetatable({}, mt4)
local success, err = pcall(obj4)
print("调用成功:", success)
print("错误信息:", err)

-- 测试5: 嵌套调用
print("\n5. 嵌套调用测试:")
local mt5 = {
    __call = function(t, func)
        print("执行传入的函数")
        return func()
    end
}

local obj5 = setmetatable({}, mt5)
local result5 = obj5(function() return "嵌套调用成功" end)
print("嵌套结果:", result5)

print("\n=== 测试完成 ===")
