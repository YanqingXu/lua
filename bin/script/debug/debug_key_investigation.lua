-- Detailed key investigation
print("=== Detailed Key Investigation ===")

print("Test 1: Member vs String key comparison")
local t = {}
t.x = 100         -- Member access set
t["x"] = 200      -- String key set
print("After setting both:")
print("t.x =", t.x)           -- Member access get
print("t['x'] =", t["x"])     -- String key get

print("Test 2: Table literal investigation")
local t2 = {x = 300}           -- Table literal
print("After table literal {x = 300}:")
print("t2.x =", t2.x)         -- Member access get
print("t2['x'] =", t2["x"])   -- String key get

print("Test 3: Iterate all keys")
print("Keys in table t:")
for k, v in pairs(t) do
    print("  key:", k, "value:", v, "key type:", type(k))
end

print("Keys in table t2:")
for k, v in pairs(t2) do
    print("  key:", k, "value:", v, "key type:", type(k))
end

print("=== Investigation completed ===")
