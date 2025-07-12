-- Test basic compilation
print("Testing basic functionality...")

-- Test basic operations
local x = 5
local y = 3
print("x + y = " .. (x + y))

-- Test table operations
local t = {}
t.key = "value"
print("t.key = " .. t.key)

-- Test metatable functions
local mt = {}
setmetatable(t, mt)
local retrieved = getmetatable(t)
print("Metatable set and retrieved successfully: " .. tostring(retrieved == mt))

print("Compilation test completed!")
