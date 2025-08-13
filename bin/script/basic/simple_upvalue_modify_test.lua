print("=== Simple Upvalue Modify Test ===")

local function make_incrementer()
    local count = 0
    return function()
        count = count + 1
        return count
    end
end

local inc = make_incrementer()
print("First call:")
local result1 = inc()
print("result1 =", result1)

print("Second call:")
local result2 = inc()
print("result2 =", result2)

print("=== Simple Upvalue Modify Test Complete ===")
