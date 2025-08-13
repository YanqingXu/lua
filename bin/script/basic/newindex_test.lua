-- __newindex metamethod test
print("=== __newindex Test ===")

-- Test 1: __newindex with table
print("Test 1: __newindex with table")
local storage = {}
local proxy = {}

local mt = {
    __index = storage,
    __newindex = storage
}

setmetatable(proxy, mt)

-- Set values through proxy
proxy.x = 10
proxy.y = 20

print("After setting through proxy:")
print("proxy.x =", proxy.x)
print("proxy.y =", proxy.y)
print("storage.x =", storage.x)
print("storage.y =", storage.y)

-- Test 2: Direct assignment to existing key
print("\nTest 2: Direct assignment")
local direct = {a = 1}
direct.a = 2
direct.b = 3

print("direct.a =", direct.a)
print("direct.b =", direct.b)

print("\n=== Test completed ===")
