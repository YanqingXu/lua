-- 第六步：测试两个连续 __call 调用
print("Step 6: Two consecutive __calls")
local obj = {}
local meta = {
    __call = function(self)
        print("Function called")
        return "result"
    end
}
setmetatable(obj, meta)

print("First call:")
local result1 = obj()
print("First result: " .. tostring(result1))

print("Second call:")
local result2 = obj()
print("Second result: " .. tostring(result2))

print("Test completed")
