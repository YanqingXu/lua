-- 第八步：测试返回函数的 __call（可能的问题源）
print("Step 8: __call returning function")

local factory = {}
local meta = {
    __call = function(self, name)
        print("Factory called with: " .. tostring(name))
        return function(action)
            print("Generated function called with: " .. tostring(action))
            return tostring(name) .. "_" .. tostring(action)
        end
    end
}
setmetatable(factory, meta)

print("Calling factory to create function:")
local generated_func = factory("Robot")
print("Generated function created")

print("Calling generated function:")
local result = generated_func("walk")
print("Result: " .. tostring(result))

print("Test completed")
