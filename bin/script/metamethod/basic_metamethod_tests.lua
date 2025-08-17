-- 基础元方法测试
-- 测试基本的元方法功能

print("=== 基础元方法测试 ===")

-- 测试1: __index元方法
print("测试1: __index元方法")
local prototype = {
    name = "原型",
    getValue = function(self) return self.value or "默认值" end
}

local obj = {}
setmetatable(obj, {__index = prototype})

print("  obj.name =", obj.name)
print("  obj:getValue() =", obj:getValue())

-- 测试2: __newindex元方法
print("\n测试2: __newindex元方法")
local storage = {}
local proxy = {}
setmetatable(proxy, {
    __newindex = function(t, k, v)
        print("    设置 " .. k .. " = " .. tostring(v))
        storage[k] = v
    end,
    __index = function(t, k)
        print("    获取 " .. k .. " = " .. tostring(storage[k]))
        return storage[k]
    end
})

proxy.test = "测试值"
local value = proxy.test

-- 测试3: __add元方法（算术运算）
print("\n测试3: __add元方法")
local Vector = {}
Vector.__index = Vector

function Vector.new(x, y)
    return setmetatable({x = x, y = y}, Vector)
end

Vector.__add = function(a, b)
    return Vector.new(a.x + b.x, a.y + b.y)
end

Vector.__tostring = function(v)
    return "(" .. v.x .. ", " .. v.y .. ")"
end

local v1 = Vector.new(1, 2)
local v2 = Vector.new(3, 4)
local v3 = v1 + v2
print("  v1 =", tostring(v1))
print("  v2 =", tostring(v2))
print("  v1 + v2 =", tostring(v3))

-- 测试4: __eq元方法（相等比较）
print("\n测试4: __eq元方法")
Vector.__eq = function(a, b)
    return a.x == b.x and a.y == b.y
end

local v4 = Vector.new(1, 2)
local v5 = Vector.new(1, 2)
local v6 = Vector.new(2, 3)

print("  v4 == v5:", v4 == v5)  -- 应该是true
print("  v4 == v6:", v4 == v6)  -- 应该是false

-- 测试5: __call元方法
print("\n测试5: __call元方法")
local callable = {}
setmetatable(callable, {
    __call = function(t, ...)
        local args = {...}
        print("    被调用，参数:", table.concat(args, ", "))
        return "返回值"
    end
})

local result = callable("参数1", "参数2")
print("  调用结果:", result)

-- 测试6: __len元方法
print("\n测试6: __len元方法")
local customLength = {1, 2, 3, 4, 5}
setmetatable(customLength, {
    __len = function(t)
        local count = 0
        for _ in pairs(t) do
            count = count + 1
        end
        return count
    end
})

print("  #customLength =", #customLength)

-- 测试7: __tostring元方法
print("\n测试7: __tostring元方法")
local person = {name = "张三", age = 25}
setmetatable(person, {
    __tostring = function(p)
        return p.name .. " (年龄: " .. p.age .. ")"
    end
})

print("  person =", tostring(person))

-- 测试8: 多个算术元方法
print("\n测试8: 多个算术元方法")
Vector.__sub = function(a, b)
    return Vector.new(a.x - b.x, a.y - b.y)
end

Vector.__mul = function(a, b)
    if type(b) == "number" then
        return Vector.new(a.x * b, a.y * b)
    else
        return Vector.new(a.x * b.x, a.y * b.y)
    end
end

local v7 = Vector.new(6, 8)
local v8 = Vector.new(2, 3)
print("  v7 - v8 =", tostring(v7 - v8))
print("  v7 * 2 =", tostring(v7 * 2))

print("\n=== 基础元方法测试完成 ===")
