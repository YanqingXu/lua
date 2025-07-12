-- 测试多个 elseif 分支
print("=== Debug ElseIf Problem ===")

local calculator = {}
setmetatable(calculator, {
    __call = function(self, op, a, b)
        print("Calculator called: " .. tostring(op))
        if op == "add" then 
            print("Branch: add")
            return a + b
        elseif op == "mul" then 
            print("Branch: mul")
            return a * b
        elseif op == "sub" then 
            print("Branch: sub")
            return a - b
        else 
            print("Branch: else")
            return 0 
        end
    end
})

print("Testing add:")
local result1 = calculator("add", 15, 25)
print("Add result: " .. tostring(result1))

print("Testing mul:")
local result2 = calculator("mul", 6, 7)
print("Mul result: " .. tostring(result2))

print("Test completed")
