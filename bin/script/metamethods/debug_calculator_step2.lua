-- 测试计算器的单次调用
print("=== Debug Calculator Step 2 ===")

local calculator = {}
setmetatable(calculator, {
    __call = function(self, op, a, b)
        print("Calculator called with: " .. tostring(op) .. ", " .. tostring(a) .. ", " .. tostring(b))
        if op == "add" then 
            print("Performing addition")
            local result = a + b
            print("Addition result: " .. tostring(result))
            return result
        else 
            print("Unknown operation")
            return 0 
        end
    end
})

print("About to call calculator...")
local add_result = calculator("add", 15, 25)
print("Call completed")
print("Result: " .. tostring(add_result))

print("Test completed")
