print("=== Minimal __index Test ===")

local proxy = {}
local mt = {
    __index = function(t, k)
        print("__index called with key:", k)
        return "value_" .. k
    end
}

print("Before setmetatable, proxy.test =", proxy.test)

setmetatable(proxy, mt)

print("After setmetatable, proxy.test =", proxy.test)

print("=== End Test ===")
