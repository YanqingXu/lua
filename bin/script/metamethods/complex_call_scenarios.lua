-- 复杂 __call 场景测试
print("=== Complex Call Scenarios Test ===")

-- 测试1: 返回函数的 __call
print("Test 1: __call returning function")
local factory = {}
local factory_meta = {
    __call = function(self, name)
        print("Factory creating: " .. tostring(name))
        return function(action)
            print("Generated function called with: " .. tostring(action))
            return tostring(name) .. "_" .. tostring(action)
        end
    end
}
setmetatable(factory, factory_meta)

print("Creating function...")
local robot_func = factory("Robot")
print("Function created, now calling it...")
local result1 = robot_func("walk")
print("Result1: " .. tostring(result1))

-- 测试2: __call 修改自身状态
print("\nTest 2: __call modifying self")
local counter = {count = 0}
local counter_meta = {
    __call = function(self)
        print("Counter before: " .. tostring(self.count))
        self.count = self.count + 1
        print("Counter after: " .. tostring(self.count))
        return self.count
    end
}
setmetatable(counter, counter_meta)

local count1 = counter()
print("Count1: " .. tostring(count1))
local count2 = counter()
print("Count2: " .. tostring(count2))

-- 测试3: 简单的嵌套对象创建（避免复杂嵌套调用）
print("\nTest 3: Simple nested object creation")
local builder = {}
local builder_meta = {
    __call = function(self, type_name)
        print("Building: " .. tostring(type_name))
        local obj = {name = type_name, value = 42}
        print("Object built with name: " .. tostring(obj.name))
        return obj
    end
}
setmetatable(builder, builder_meta)

local built_obj = builder("TestObject")
print("Built object name: " .. tostring(built_obj.name))
print("Built object value: " .. tostring(built_obj.value))

print("\n=== Complex scenarios test completed ===")
