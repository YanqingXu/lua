print("=== Working __index Test ===")

local proxy = {}
local mt = {
    __index = function(t, k)
        print("__index called!")
        print("table:", t)
        print("key:", k)
        if k == "test" then
            return "success!"
        else
            return "unknown_key"
        end
    end
}

setmetatable(proxy, mt)

print("Accessing proxy.test...")
local result = proxy.test
print("Result:", result)

print("Accessing proxy.other...")
local result2 = proxy.other
print("Result2:", result2)

print("=== Test Complete ===")
