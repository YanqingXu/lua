print("=== Clean __index Test ===")

-- Create completely fresh tables
local data = {alpha = 100, beta = 200}
local proxy = {}

print("Before setmetatable:")
print("  proxy.alpha =", proxy.alpha)
print("  proxy.beta =", proxy.beta)

-- Create metatable with __index function
local mt = {
    __index = function(t, k)
        print("__index function called!")
        print("  table:", t)
        print("  key:", k)
        return data[k] or "not_found"
    end
}

-- Set metatable
setmetatable(proxy, mt)

print("\nAfter setmetatable:")
print("Testing proxy.alpha:")
local val1 = proxy.alpha
print("Result:", val1)

print("\nTesting proxy.beta:")
local val2 = proxy.beta
print("Result:", val2)

print("\nTesting proxy.gamma:")
local val3 = proxy.gamma
print("Result:", val3)

print("\n=== Clean Test Complete ===")
