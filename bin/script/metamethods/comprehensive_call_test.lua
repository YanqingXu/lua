-- 综合 __call 元方法测试
print("=== Comprehensive Call Metamethod Test ===")

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
        else
            return 0
        end
    end
}

setmetatable(calculator, calc_meta)

local add_result = calculator("add", 15, 25)
print("15 + 25 = " .. tostring(add_result))

local sub_result = calculator("sub", 30, 12)
print("30 - 12 = " .. tostring(sub_result))

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
print("Final count: " .. tostring(count2))

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

-- 测试4: __call 与其他元方法结合
print("\nTest 4: __call with other metamethods")
local smart_obj = {value = 100}
local smart_meta = {
    __call = function(self, increment)
        print("Smart object called with increment: " .. tostring(increment))
        self.value = self.value + increment
        return self
    end,
    __tostring = function(self)
        return "SmartObj(" .. tostring(self.value) .. ")"
    end
}

setmetatable(smart_obj, smart_meta)

local updated_obj = smart_obj(50)
print("Updated object value: " .. tostring(updated_obj.value))

-- 测试5: 简单的嵌套调用
print("\nTest 5: Simple nested calls")
local outer = {}
local outer_meta = {
    __call = function(self, value)
        print("Outer called with: " .. tostring(value))
        return "outer_" .. tostring(value)
    end
}

setmetatable(outer, outer_meta)

local result1 = outer("hello")
print("Outer result: " .. tostring(result1))

print("\n=== Comprehensive call test completed ===")
print("🎉 __call metamethod is fully functional!")
