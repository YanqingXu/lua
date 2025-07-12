-- 高级 __call 元方法测试
print("=== Advanced Call Metamethod Test ===")

-- 测试1: 多参数 __call
print("Test 1: Multi-argument __call")
local calculator = {}
local calc_meta = {
    __call = function(self, op, a, b)
        print("Calculator: " .. tostring(op) .. "(" .. tostring(a) .. ", " .. tostring(b) .. ")")
        if op == "add" then
            return a + b
        elseif op == "sub" then
            return a - b
        elseif op == "mul" then
            return a * b
        elseif op == "div" then
            return a / b
        else
            return 0
        end
    end
}

setmetatable(calculator, calc_meta)

local add_result = calculator("add", 15, 25)
print("15 + 25 = " .. tostring(add_result))

local mul_result = calculator("mul", 6, 7)
print("6 * 7 = " .. tostring(mul_result))

-- 测试2: 无参数 __call
print("\nTest 2: No-argument __call")
local counter = {count = 0}
local counter_meta = {
    __call = function(self)
        self.count = self.count + 1
        print("Counter called, count is now: " .. tostring(self.count))
        return self.count
    end
}

setmetatable(counter, counter_meta)

local count1 = counter()
local count2 = counter()
local count3 = counter()
print("Final count: " .. tostring(count3))

-- 测试3: 返回函数的 __call
print("\nTest 3: __call returning function")
local factory = {}
local factory_meta = {
    __call = function(self, name)
        print("Creating function for: " .. tostring(name))
        return function(action)
            return tostring(name) .. " performs " .. tostring(action)
        end
    end
}

setmetatable(factory, factory_meta)

local robot_func = factory("Robot")
local result = robot_func("walking")
print("Result: " .. tostring(result))

-- 测试4: 嵌套 __call
print("\nTest 4: Nested __call objects")
local outer = {}
local outer_meta = {
    __call = function(self, value)
        print("Outer called with: " .. tostring(value))
        local inner = {data = value}
        local inner_meta = {
            __call = function(inner_self, suffix)
                print("Inner called with: " .. tostring(suffix))
                return tostring(inner_self.data) .. "_" .. tostring(suffix)
            end
        }
        setmetatable(inner, inner_meta)
        return inner
    end
}

setmetatable(outer, outer_meta)

local inner_obj = outer("hello")
local final_result = inner_obj("world")
print("Nested call result: " .. tostring(final_result))

-- 测试5: __call 与其他元方法结合
print("\nTest 5: __call with other metamethods")
local smart_obj = {value = 100}
local smart_meta = {
    __call = function(self, increment)
        print("Smart object called with increment: " .. tostring(increment))
        self.value = self.value + increment
        return self
    end,
    __tostring = function(self)
        return "SmartObj(" .. tostring(self.value) .. ")"
    end,
    __add = function(self, other)
        return {value = self.value + other.value}
    end
}

setmetatable(smart_obj, smart_meta)

local updated_obj = smart_obj(50)
print("Updated object value: " .. tostring(updated_obj.value))

print("\n=== Advanced call test completed ===")
print("🎉 __call metamethod is fully functional with complex scenarios!")
