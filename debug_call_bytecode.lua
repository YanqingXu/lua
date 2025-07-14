-- 测试字节码生成
print("=== Bytecode Debug Test ===")

-- 测试普通函数调用
print("\nTest 1: Normal function call")
local function normalFunc(x)
    print("Normal function called with:", x)
    return x * 2
end

local result1 = normalFunc(5)
print("Normal function result:", result1)

-- 测试__call元方法
print("\nTest 2: __call metamethod")
local obj = {}
setmetatable(obj, {
    __call = function(self, x)
        print("__call metamethod called")
        print("Arguments received:")
        print("  self:", self)
        print("  x:", x)
        return x * 3
    end
})

local result2 = obj(5)
print("__call result:", result2)

print("\n=== Test completed ===")
