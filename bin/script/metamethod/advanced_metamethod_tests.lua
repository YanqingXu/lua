-- 高级元方法测试
-- 测试更复杂的元方法功能和组合

print("=== 高级元方法测试 ===")

-- 测试1: __lt和__le元方法（比较运算）
print("测试1: __lt和__le元方法")
local Number = {}
Number.__index = Number

function Number.new(value)
    return setmetatable({value = value}, Number)
end

Number.__lt = function(a, b)
    return a.value < b.value
end

Number.__le = function(a, b)
    return a.value <= b.value
end

Number.__tostring = function(n)
    return tostring(n.value)
end

local n1 = Number.new(5)
local n2 = Number.new(10)
local n3 = Number.new(5)

print("  n1 =", tostring(n1))
print("  n2 =", tostring(n2))
print("  n3 =", tostring(n3))
print("  n1 < n2:", n1 < n2)
print("  n2 < n1:", n2 < n1)
print("  n1 <= n3:", n1 <= n3)

-- 测试2: __concat元方法（字符串连接）
print("\n测试2: __concat元方法")
local StringWrapper = {}
StringWrapper.__index = StringWrapper

function StringWrapper.new(str)
    return setmetatable({str = str}, StringWrapper)
end

StringWrapper.__concat = function(a, b)
    local aStr = type(a) == "table" and a.str or tostring(a)
    local bStr = type(b) == "table" and b.str or tostring(b)
    return StringWrapper.new(aStr .. bStr)
end

StringWrapper.__tostring = function(s)
    return s.str
end

local s1 = StringWrapper.new("Hello")
local s2 = StringWrapper.new(" World")
local s3 = s1 .. s2
print("  s1 =", tostring(s1))
print("  s2 =", tostring(s2))
print("  s1 .. s2 =", tostring(s3))

-- 测试3: __unm元方法（一元负号）
print("\n测试3: __unm元方法")
local SignedNumber = {}
SignedNumber.__index = SignedNumber

function SignedNumber.new(value)
    return setmetatable({value = value}, SignedNumber)
end

SignedNumber.__unm = function(n)
    return SignedNumber.new(-n.value)
end

SignedNumber.__tostring = function(n)
    return tostring(n.value)
end

local sn1 = SignedNumber.new(42)
local sn2 = -sn1
print("  sn1 =", tostring(sn1))
print("  -sn1 =", tostring(sn2))

-- 测试4: __div和__mod元方法
print("\n测试4: __div和__mod元方法")
SignedNumber.__div = function(a, b)
    return SignedNumber.new(a.value / b.value)
end

SignedNumber.__mod = function(a, b)
    return SignedNumber.new(a.value % b.value)
end

local sn3 = SignedNumber.new(10)
local sn4 = SignedNumber.new(3)
print("  sn3 / sn4 =", tostring(sn3 / sn4))
print("  sn3 % sn4 =", tostring(sn3 % sn4))

-- 测试5: __pow元方法（幂运算）
print("\n测试5: __pow元方法")
SignedNumber.__pow = function(a, b)
    return SignedNumber.new(a.value ^ b.value)
end

local sn5 = SignedNumber.new(2)
local sn6 = SignedNumber.new(3)
print("  sn5 ^ sn6 =", tostring(sn5 ^ sn6))

-- 测试6: 复杂的__index链
print("\n测试6: 复杂的__index链")
local base = {
    baseMethod = function() return "来自base" end
}

local middle = {}
setmetatable(middle, {__index = base})
middle.middleMethod = function() return "来自middle" end

local derived = {}
setmetatable(derived, {__index = middle})
derived.derivedMethod = function() return "来自derived" end

print("  derived.derivedMethod():", derived.derivedMethod())
print("  derived.middleMethod():", derived.middleMethod())
print("  derived.baseMethod():", derived.baseMethod())

-- 测试7: __index和__newindex的组合
print("\n测试7: __index和__newindex的组合")
local ReadOnlyTable = {}

function ReadOnlyTable.new(data)
    local instance = {}
    setmetatable(instance, {
        __index = function(t, k)
            return data[k]
        end,
        __newindex = function(t, k, v)
            error("尝试修改只读表的键: " .. tostring(k))
        end
    })
    return instance
end

local readOnly = ReadOnlyTable.new({a = 1, b = 2, c = 3})
print("  readOnly.a =", readOnly.a)
print("  readOnly.b =", readOnly.b)

-- 尝试修改（应该出错）
print("  尝试修改只读表...")
local success, err = pcall(function()
    readOnly.a = 10
end)
if not success then
    print("  ✓ 正确阻止了修改:", err)
else
    print("  ✗ 没有阻止修改")
end

-- 测试8: 元方法的递归调用
print("\n测试8: 元方法的递归调用")
local RecursiveTable = {}
setmetatable(RecursiveTable, {
    __index = function(t, k)
        if k == "recursive" then
            return function(depth)
                if depth <= 0 then
                    return "递归结束"
                else
                    return "递归深度" .. depth .. " -> " .. t.recursive(depth - 1)
                end
            end
        end
        return nil
    end
})

print("  RecursiveTable.recursive(3):", RecursiveTable.recursive(3))

print("\n=== 高级元方法测试完成 ===")
