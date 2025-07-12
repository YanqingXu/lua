-- 测试计算器的创建
print("=== Debug Calculator Step 1 ===")

print("Creating calculator object...")
local calculator = {}
print("Calculator object created")

print("Creating metatable...")
local meta = {
    __call = function(self, op, a, b)
        print("Calculator called with: " .. tostring(op) .. ", " .. tostring(a) .. ", " .. tostring(b))
        if op == "add" then 
            print("Performing addition")
            return a + b
        else 
            print("Unknown operation")
            return 0 
        end
    end
}
print("Metatable created")

print("Setting metatable...")
setmetatable(calculator, meta)
print("Metatable set")

print("Test completed")
