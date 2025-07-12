-- Final validation test
print("=== Final Validation Test ===")

-- The original problematic case
local t = {x = 10, y = 20}
print("t.x =", t.x)
print("t.y =", t.y)

-- Mixed table
local mixed = {1, 2, name = "test", value = 42}
print("mixed[1] =", mixed[1]) 
print("mixed[2] =", mixed[2])
print("mixed.name =", mixed.name)
print("mixed.value =", mixed.value)

-- Dynamic assignment comparison
local dynamic = {}
dynamic.x = 10
dynamic.y = 20
print("dynamic.x =", dynamic.x)
print("dynamic.y =", dynamic.y)

print("=== All tests passed! ===")
