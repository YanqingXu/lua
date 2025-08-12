-- Simple metatable test
print("=== Simple Metatable Test ===")

-- Test basic setmetatable
local t = {}
local mt = {}

print("Before setmetatable:")
print("t =", t)
print("mt =", mt)

local result = setmetatable(t, mt)
print("setmetatable result =", result)

local retrieved = getmetatable(t)
print("getmetatable result =", retrieved)

print("=== Test completed ===")
