print("=== Final Metatable Test ===")

-- Test 1: Basic setmetatable/getmetatable
print("Test 1: Basic metatable operations")
local t = {}
local mt = {}

print("Before setmetatable:")
print("  t =", t, "type:", type(t))
print("  mt =", mt, "type:", type(mt))

local result = setmetatable(t, mt)
print("After setmetatable:")
print("  result =", result, "type:", type(result))
print("  result == t:", result == t)

local retrieved_mt = getmetatable(t)
print("After getmetatable:")
print("  retrieved_mt =", retrieved_mt, "type:", type(retrieved_mt))
print("  retrieved_mt == mt:", retrieved_mt == mt)

-- Test 2: __index metamethod with table (this should work)
print("\nTest 2: __index metamethod (table)")
local defaults = {a = 1, b = 2, c = 3}
local config = {}
local config_mt = {__index = defaults}
setmetatable(config, config_mt)

print("config.a =", config.a)
print("config.b =", config.b)
print("config.c =", config.c)

-- Test 3: __index metamethod with function (this might not work yet)
print("\nTest 3: __index metamethod (function)")
local data = {x = 10, y = 20}
local proxy = {}
local proxy_mt = {
    __index = function(table, key)
        print("  __index function called with key:", key)
        return data[key]
    end
}
setmetatable(proxy, proxy_mt)

print("proxy.x =", proxy.x)
print("proxy.y =", proxy.y)
print("proxy.z =", proxy.z)

print("\n=== Final Metatable Test Completed ===")
