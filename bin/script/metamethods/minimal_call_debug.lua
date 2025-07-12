-- 最简单的 __call 测试
print("Starting minimal call test")

local obj = {}
print("Object created")

local meta = {
    __call = function(self, arg)
        print("__call executed")
        return "result"
    end
}
print("Metatable created")

setmetatable(obj, meta)
print("Metatable set")

print("About to call obj")
local result = obj("test")
print("Call completed")
print("Result: " .. tostring(result))

print("Test finished")
