print("=== Debug setmetatable/getmetatable ===")

local t = {name = "test"}
local mt = {__name = "metatable"}

print("Before setmetatable:")
print("t =", t)
print("mt =", mt)

print("\nCalling setmetatable(t, mt)...")
local result = setmetatable(t, mt)
print("setmetatable returned:", result)
print("type of result:", type(result))

print("\nCalling getmetatable(t)...")
local retrieved = getmetatable(t)
print("getmetatable returned:", retrieved)
print("type of retrieved:", type(retrieved))

if retrieved then
    print("retrieved.__name =", retrieved.__name)
end

print("\nChecking equality:")
print("result == t:", result == t)
print("retrieved == mt:", retrieved == mt)
