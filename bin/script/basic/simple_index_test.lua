print("=== Simple __index Test ===")

-- Create a table with __index metamethod
local data = {x = 42}
local proxy = {}

local mt = {
    __index = function(t, k)
        print("__index function called with key:", k)
        return data[k]
    end
}

setmetatable(proxy, mt)

print("About to access proxy.x...")
local value = proxy.x
print("proxy.x =", value)

print("About to access proxy['y']...")
local value2 = proxy["y"]
print("proxy['y'] =", value2)

print("=== Test Complete ===")
