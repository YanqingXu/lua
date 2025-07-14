-- 测试__call元方法的多返回值功能
print("=== __call Multi-Return Value Test ===")

-- 测试1: 基本多返回值
print("\nTest 1: Basic multi-return values")
local obj1 = {}
setmetatable(obj1, {
    __call = function(self, x)
        print("__call called with x =", x)
        print("Returning:", x, x*2, x*3)
        return x, x*2, x*3
    end
})

print("Calling obj1(5)...")
local a, b, c = obj1(5)
print("Results: a =", a, "b =", b, "c =", c)
print("Expected: a = 5, b = 10, c = 15")

-- 测试2: 不同数量的返回值
print("\nTest 2: Different number of return values")
local obj2 = {}
setmetatable(obj2, {
    __call = function(self, count)
        if count == 1 then
            return "one"
        elseif count == 2 then
            return "first", "second"
        elseif count == 3 then
            return "alpha", "beta", "gamma"
        else
            return "default"
        end
    end
})

print("Testing 1 return value:")
local r1 = obj2(1)
print("Result:", r1)

print("Testing 2 return values:")
local r2a, r2b = obj2(2)
print("Results:", r2a, r2b)

print("Testing 3 return values:")
local r3a, r3b, r3c = obj2(3)
print("Results:", r3a, r3b, r3c)

-- 测试3: 返回值顺序验证
print("\nTest 3: Return value order verification")
local obj3 = {}
setmetatable(obj3, {
    __call = function(self)
        print("Returning in order: 'first', 'second', 'third'")
        return "first", "second", "third"
    end
})

local first, second, third = obj3()
print("Received: first =", first, "second =", second, "third =", third)

-- 测试4: 数值返回值
print("\nTest 4: Numeric return values")
local obj4 = {}
setmetatable(obj4, {
    __call = function(self, base)
        return base, base + 1, base + 2, base + 3
    end
})

local n1, n2, n3, n4 = obj4(10)
print("Numeric results:", n1, n2, n3, n4)
print("Expected: 10, 11, 12, 13")

print("\n=== Test completed ===")
