-- 测试简化的计算器逻辑
print("=== Debug Calculator Simple ===")

local calculator = {}
setmetatable(calculator, {
    __call = function(self, op, a, b)
        print("Calculator called: " .. tostring(op))
        if op == "add" then 
            print("Doing addition")
            return a + b
        else 
            print("Unknown operation")
            return 0 
        end
    end
})

print("First call (add):")
local result1 = calculator("add", 15, 25)
print("First result: " .. tostring(result1))

print("Second call (unknown):")
local result2 = calculator("unknown", 1, 2)
print("Second result: " .. tostring(result2))

print("Test completed")
