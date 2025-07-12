-- 测试计算器的两次连续调用
print("=== Debug Calculator Step 3 ===")

local calculator = {}
setmetatable(calculator, {
    __call = function(self, op, a, b)
        print("Calculator called with: " .. tostring(op) .. ", " .. tostring(a) .. ", " .. tostring(b))
        if op == "add" then 
            return a + b
        elseif op == "mul" then 
            return a * b
        else 
            return 0 
        end
    end
})

print("First call:")
local add_result = calculator("add", 15, 25)
print("First result: " .. tostring(add_result))

print("Second call:")
local mul_result = calculator("mul", 6, 7)
print("Second result: " .. tostring(mul_result))

print("Test completed")
