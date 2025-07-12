-- 第三步：测试函数创建
print("Step 3: Function creation")
local obj = {}
local meta = {
    __call = function(self)
        print("Function called")
        return "result"
    end
}
print("Function created in metatable")
print("Test completed")
