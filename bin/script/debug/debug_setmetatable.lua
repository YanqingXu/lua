print("=== Debug setmetatable ===")

-- Create separate tables
local data = {x = 42}
local proxy = {}

print("Before setmetatable:")
print("  proxy.x =", proxy.x)
print("  data.x =", data.x)

-- Create metatable
local mt = {
    __index = function(t, k)
        print("  __index function called with key:", k)
        return data[k]
    end
}

print("Setting metatable...")
local result = setmetatable(proxy, mt)
print("setmetatable returned:", result == proxy)

print("After setmetatable:")
print("  proxy.x =", proxy.x)

-- Check if proxy actually has x directly
print("Raw proxy contents:")
for k, v in pairs(proxy) do
    print("  proxy[" .. tostring(k) .. "] =", v)
end

print("=== End Debug ===")
