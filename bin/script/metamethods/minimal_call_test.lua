-- Minimal __call test
print("=== Minimal __call Test ===")

local obj = {}
setmetatable(obj, {
    __call = function(self, x)
        return x * 2
    end
})

print("Testing obj(21)...")
local result = obj(21)
print("Result:", result)

print("=== Test completed ===")
