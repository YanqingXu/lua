-- 第七步：测试多个对象的 __call
print("Step 7: Multiple objects with __call")

local obj1 = {}
local meta1 = {
    __call = function(self)
        print("Object 1 called")
        return "result1"
    end
}
setmetatable(obj1, meta1)

local obj2 = {}
local meta2 = {
    __call = function(self)
        print("Object 2 called")
        return "result2"
    end
}
setmetatable(obj2, meta2)

print("Calling obj1:")
local result1 = obj1()
print("Result1: " .. tostring(result1))

print("Calling obj2:")
local result2 = obj2()
print("Result2: " .. tostring(result2))

print("Test completed")
