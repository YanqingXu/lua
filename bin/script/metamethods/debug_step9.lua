-- 第九步：测试在 __call 中创建新的可调用对象（潜在问题源）
print("Step 9: __call creating callable objects")

local factory = {}
local factory_meta = {
    __call = function(self, name)
        print("Factory creating: " .. tostring(name))
        
        -- 创建一个新的可调用对象
        local obj = {name = name}
        local obj_meta = {
            __call = function(obj_self, action)
                print("Created object called: " .. tostring(obj_self.name) .. " -> " .. tostring(action))
                return tostring(obj_self.name) .. "_" .. tostring(action)
            end
        }
        setmetatable(obj, obj_meta)
        
        print("Object created and metatable set")
        return obj
    end
}
setmetatable(factory, factory_meta)

print("Step 1: Creating object via factory")
local robot = factory("Robot")
print("Robot created")

print("Step 2: Calling the created object")
local result = robot("walk")
print("Result: " .. tostring(result))

print("Test completed")
