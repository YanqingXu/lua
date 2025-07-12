-- 第五步：测试单个 __call 调用
print("Step 5: Single __call")
local obj = {}
local meta = {
    __call = function(self)
        print("Function called")
        return "result"
    end
}
setmetatable(obj, meta)
print("About to call obj()")
local result = obj()
print("Call completed")
print("Result: " .. tostring(result))
print("Test completed")
