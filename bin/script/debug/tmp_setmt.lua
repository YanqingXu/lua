local t = {}
local mt = {}
local r = setmetatable(t, mt)
print("r is t?", r == t)
local g = getmetatable(t)
print("getmetatable ok?", g == mt)

