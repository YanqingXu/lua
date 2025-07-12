-- Step by step test
print("=== Step by Step Test ===")

-- Step 1: Basic print
print("Step 1: Basic print works")

-- Step 2: Variable assignment
local obj = {}
print("Step 2: Created empty table")

-- Step 3: Table assignment
obj.existing = "original"
print("Step 3: Set obj.existing")

-- Step 4: Access table value
print("Step 4: obj.existing = " .. obj.existing)

-- Step 5: Test getmetatable
print("Step 5: Testing getmetatable")
local result = getmetatable(obj)
print("Step 6: getmetatable returned")

-- Step 6: Check result
if result == nil then
    print("Step 7: getmetatable returned nil (expected)")
else
    print("Step 7: getmetatable returned something: " .. tostring(result))
end

print("=== Step by step test completed ===")
