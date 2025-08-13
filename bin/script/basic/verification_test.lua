print("=== Metatable Verification Test ===")

-- Test 1: Basic setmetatable/getmetatable
local t = {}
local mt = {}
local result = setmetatable(t, mt)
local retrieved = getmetatable(t)

print("Test 1 - Basic operations:")
print("  setmetatable works:", result == t)
print("  getmetatable works:", retrieved == mt)

-- Test 2: __index table form
local defaults = {value = 42}
local config = {}
setmetatable(config, {__index = defaults})

print("\nTest 2 - __index table:")
print("  config.value =", config.value)

-- Test 3: __index function form
local proxy = {}
setmetatable(proxy, {
    __index = function(table, key)
        if key == "test" then
            return "success"
        else
            return "unknown"
        end
    end
})

print("\nTest 3 - __index function:")
print("  proxy.test =", proxy.test)
print("  proxy.other =", proxy.other)

print("\n=== All Tests Complete ===")
