-- 第四步：测试 setmetatable
print("Step 4: setmetatable")
local obj = {}
local meta = {
    __call = function(self)
        print("Function called")
        return "result"
    end
}
print("About to call setmetatable")
setmetatable(obj, meta)
print("setmetatable completed")
print("Test completed")
