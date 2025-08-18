-- Basic metamethod tests
-- Test basic metamethod functionality

print("=== Basic Metamethod Tests ===")

-- Test 1: __index metamethod
print("Test 1: __index metamethod")
local prototype = {
    name = "prototype",
    getValue = function(self) return self.value or "default value" end
}

local obj = {}
setmetatable(obj, {__index = prototype})

print("  obj.name =", obj.name)
print("  obj:getValue() =", obj:getValue())

-- Test 2: __newindex metamethod
print("\nTest 2: __newindex metamethod")
local storage = {}
local proxy = {}
setmetatable(proxy, {
    __newindex = function(t, k, v)
        print("    Setting " .. k .. " = " .. tostring(v))
        storage[k] = v
    end,
    __index = function(t, k)
        print("    Getting " .. k .. " = " .. tostring(storage[k]))
        return storage[k]
    end
})

proxy.test = "test value"
local value = proxy.test

-- Test 3: __add metamethod (arithmetic operation)
print("\nTest 3: __add metamethod")
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

-- Test 4: __eq metamethod (equality comparison)
print("\nTest 4: __eq metamethod")
Vector.__eq = function(a, b)
    return a.x == b.x and a.y == b.y
end

local v4 = Vector.new(1, 2)
local v5 = Vector.new(1, 2)
local v6 = Vector.new(2, 3)

print("  v4 == v5:", v4 == v5)  -- Should be true
print("  v4 == v6:", v4 == v6)  -- Should be false

-- Test 5: __call metamethod
print("\nTest 5: __call metamethod")
local callable = {}
setmetatable(callable, {
    __call = function(t, ...)
        local args = {...}
        print("    Called with arguments:", table.concat(args, ", "))
        return "return value"
    end
})

local result = callable("arg1", "arg2")
print("  Call result:", result)

-- Test 6: __len metamethod
print("\nTest 6: __len metamethod")
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

-- Test 7: __tostring metamethod
print("\nTest 7: __tostring metamethod")
local person = {name = "John", age = 25}
setmetatable(person, {
    __tostring = function(p)
        return p.name .. " (age: " .. p.age .. ")"
    end
})

print("  person =", tostring(person))

-- Test 8: Multiple arithmetic metamethods
print("\nTest 8: Multiple arithmetic metamethods")
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

print("\n=== Basic Metamethod Tests Complete ===")
