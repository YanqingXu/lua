-- 测试两个分支
print("=== Debug Two Branches ===")

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
        else 
            print("Branch: else")
            return 0 
        end
    end
})

print("Testing add:")
local result1 = calculator("add", 15, 25)
print("Add result: " .. tostring(result1))

print("Test completed")
