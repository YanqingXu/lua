-- 最终的__call元方法测试（避免使用...语法）
print("=== 最终__call元方法测试 ===")

-- 测试1: 基本调用
print("\n1. 基本调用:")
local mt1 = {
    __call = function(t)
        return "基本调用成功"
    end
}
local obj1 = setmetatable({}, mt1)
print("结果:", obj1())

-- 测试2: 带参数调用
print("\n2. 带参数调用:")
local mt2 = {
    __call = function(t, a, b)
        return a + b
    end
}
local obj2 = setmetatable({}, mt2)
print("结果:", obj2(10, 20))

-- 测试3: 访问self
print("\n3. 访问self:")
local mt3 = {
    __call = function(self, name)
        return "Hello " .. name .. ", value=" .. (self.value or "nil")
    end
}
local obj3 = setmetatable({value = 42}, mt3)
print("结果:", obj3("World"))

-- 测试4: 多返回值
print("\n4. 多返回值:")
local mt4 = {
    __call = function(t, x)
        return x, x * 2, x * 3
    end
}
local obj4 = setmetatable({}, mt4)
local a, b, c = obj4(5)
print("结果:", a, b, c)

-- 测试5: 错误处理
print("\n5. 错误处理:")
local mt5 = {
    __call = function(t)
        error("测试错误")
    end
}
local obj5 = setmetatable({}, mt5)
local success, err = pcall(obj5)
print("成功:", success)
print("错误:", err)

print("\n=== 测试完成 ===")
