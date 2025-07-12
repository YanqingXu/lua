-- Debug member access vs table literal
print("=== Member Access Debug ===")

-- Test 1: Create table dynamically and access
print("\nTest 1: Dynamic table creation")
local t1 = {}
t1.x = 10
t1.y = 20
print("t1.x =", t1.x)
print("t1.y =", t1.y)

-- Test 2: Create table with literal and access
print("\nTest 2: Table literal creation")
local t2 = {x = 10, y = 20}
print("t2.x =", t2.x)
print("t2.y =", t2.y)

-- Test 3: Compare string access
print("\nTest 3: String access comparison")
print("t1['x'] =", t1['x'])
print("t2['x'] =", t2['x'])

-- Test 4: Check table structure
print("\nTest 4: Table structure")
print("t1 pairs:")
for k, v in pairs(t1) do
    print("  [" .. tostring(k) .. "] = " .. tostring(v))
end

print("t2 pairs:")
for k, v in pairs(t2) do
    print("  [" .. tostring(k) .. "] = " .. tostring(v))
end

print("\n=== Debug completed ===")
