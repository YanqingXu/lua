-- __call 元方法修复测试
print("=== Call Metamethod Fixed Test ===")

-- 测试1: 简单的 __call 元方法
print("Test 1: Simple __call metamethod")
local callable = {}
local meta = {
    __call = function(self, arg1, arg2)
        print("__call invoked with args: " .. tostring(arg1) .. ", " .. tostring(arg2))
        return "Result: " .. tostring(arg1) .. " + " .. tostring(arg2)
    end
}

setmetatable(callable, meta)

print("Metatable set successfully")
print("__call metamethod exists: " .. tostring(getmetatable(callable).__call ~= nil))

-- 测试2: 尝试调用 __call 元方法
print("\nTest 2: Attempting to call __call metamethod")
print("Calling callable('hello', 'world'):")

-- 这里应该触发 __call 元方法
local result = callable("hello", "world")
print("Call result: " .. tostring(result))

-- 测试3: 带数字参数的 __call
print("\nTest 3: __call with numeric arguments")
local calculator = {}
local calc_meta = {
    __call = function(self, operation, a, b)
        print("Calculator called: " .. tostring(operation) .. "(" .. tostring(a) .. ", " .. tostring(b) .. ")")
        if operation == "add" then
            return a + b
        elseif operation == "mul" then
            return a * b
        else
            return 0
        end
    end
}

setmetatable(calculator, calc_meta)

print("Calling calculator('add', 10, 20):")
local add_result = calculator("add", 10, 20)
print("Addition result: " .. tostring(add_result))

print("Calling calculator('mul', 5, 6):")
local mul_result = calculator("mul", 5, 6)
print("Multiplication result: " .. tostring(mul_result))

-- 测试4: 无参数的 __call
print("\nTest 4: __call with no arguments")
local greeter = {}
local greet_meta = {
    __call = function(self)
        print("Hello from __call!")
        return "greeting"
    end
}

setmetatable(greeter, greet_meta)

print("Calling greeter():")
local greet_result = greeter()
print("Greeting result: " .. tostring(greet_result))

-- 测试5: 嵌套调用
print("\nTest 5: Nested __call")
local factory = {}
local factory_meta = {
    __call = function(self, name)
        print("Factory creating: " .. tostring(name))
        local obj = {name = name}
        local obj_meta = {
            __call = function(obj_self, action)
                print(tostring(obj_self.name) .. " performing: " .. tostring(action))
                return tostring(obj_self.name) .. "_" .. tostring(action)
            end
        }
        setmetatable(obj, obj_meta)
        return obj
    end
}

setmetatable(factory, factory_meta)

print("Creating object with factory('robot'):")
local robot = factory("robot")
print("Robot created: " .. tostring(robot.name))

print("Calling robot('walk'):")
local action_result = robot("walk")
print("Action result: " .. tostring(action_result))

print("\n=== Call metamethod test completed ===")
print("If all calls above worked, __call metamethod is fully functional!")
