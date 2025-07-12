-- Step 2: Table literal initialization test
print("=== Step 2: Table Literal Test ===")

-- Test A: Empty table
print("Test A: Empty table")
local empty = {}
print("Created empty table: " .. tostring(empty))

-- Test B: Table with string key-value pair
print("Test B: Table with string key-value pair")
local obj = {name = "test"}
print("Created obj = {name = 'test'}")

-- Test C: Access the value
print("Test C: Access the value")
local name = obj.name
if name == nil then
    print("ERROR: obj.name is nil!")
else
    print("SUCCESS: obj.name = " .. name)
end

-- Test D: Table with multiple fields
print("Test D: Table with multiple fields")
local multi = {x = 10, y = 20, name = "point"}
print("Created multi = {x = 10, y = 20, name = 'point'}")

-- Test E: Access multiple values
print("Test E: Access multiple values")
print("multi.x = " .. tostring(multi.x))
print("multi.y = " .. tostring(multi.y))
print("multi.name = " .. tostring(multi.name))

-- Test F: Table with numeric values
print("Test F: Table with numeric values")
local nums = {value = 42, count = 100}
print("Created nums = {value = 42, count = 100}")
print("nums.value = " .. tostring(nums.value))
print("nums.count = " .. tostring(nums.count))

print("=== Step 2 completed ===")
