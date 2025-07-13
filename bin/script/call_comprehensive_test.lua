-- 全面的__call测试
print("=== 全面__call测试 ===")

-- 测试1: 无参数调用
print("\n1. 无参数调用:")
local mt1 = {
    __call = function(t)
        return "无参数调用成功"
    end
}
local obj1 = setmetatable({}, mt1)
print("结果:", obj1())

-- 测试2: 多参数调用
print("\n2. 多参数调用:")
local mt2 = {
    __call = function(t, a, b, c)
        return a + b + c
    end
}
local obj2 = setmetatable({}, mt2)
print("结果:", obj2(1, 2, 3))

-- 测试3: 可变参数
print("\n3. 可变参数调用:")
local mt3 = {
    __call = function(t, ...)
        local sum = 0
        for i = 1, select("#", ...) do
            sum = sum + select(i, ...)
        end
        return sum
    end
}
local obj3 = setmetatable({}, mt3)
print("结果:", obj3(1, 2, 3, 4, 5))

-- 测试4: 访问表数据
print("\n4. 访问表数据:")
local mt4 = {
    __call = function(self, multiplier)
        return self.value * multiplier
    end
}
local obj4 = setmetatable({value = 10}, mt4)
print("结果:", obj4(3))

-- 测试5: 返回多个值
print("\n5. 返回多个值:")
local mt5 = {
    __call = function(t, x)
        return x, x * 2, x * 3
    end
}
local obj5 = setmetatable({}, mt5)
local a, b, c = obj5(4)
print("结果:", a, b, c)

print("\n=== 测试完成 ===")
