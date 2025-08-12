-- Metatable functionality test
print("=== Metatable Test ===")

-- Test 1: Basic setmetatable/getmetatable
print("Test 1: Basic metatable operations")
local t = {}
local mt = {}

-- Set metatable
local result = setmetatable(t, mt)
print("setmetatable returned:", result == t)

-- Get metatable
local retrieved_mt = getmetatable(t)
print("getmetatable works:", retrieved_mt == mt)

-- Test 2: __index metamethod with function
print("\nTest 2: __index metamethod (function)")
local data = {x = 10, y = 20}
local proxy = {}

local proxy_mt = {
    __index = function(table, key)
        print("__index called with key:", key)
        return data[key]
    end
}

setmetatable(proxy, proxy_mt)

print("proxy.x =", proxy.x)
print("proxy.y =", proxy.y)
print("proxy.z =", proxy.z)

-- Test 3: __index metamethod with table
print("\nTest 3: __index metamethod (table)")
local defaults = {a = 1, b = 2, c = 3}
local config = {}

local config_mt = {
    __index = defaults
}

setmetatable(config, config_mt)

print("config.a =", config.a)
print("config.b =", config.b)
print("config.c =", config.c)

print("\n=== Metatable Test Completed ===")
