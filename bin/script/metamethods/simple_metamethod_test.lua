-- Simple metamethod test
print("=== Simple Metamethod Test ===")

-- Test 1: Basic table creation
print("Test 1: Basic table creation")
local obj = {}
obj.name = "TestObject"
print("Created obj with obj.name = " .. obj.name)

-- Test 2: Create metatable
print("")
print("Test 2: Create metatable")
local meta = {}
meta.type = "MetaTable"
print("Created meta with meta.type = " .. meta.type)

-- Test 3: Test getmetatable before setting (should be nil)
print("")
print("Test 3: getmetatable before setting")
local result = getmetatable(obj)
if result == nil then
    print("getmetatable(obj) = nil (correct)")
else
    print("getmetatable(obj) = " .. tostring(result))
end

-- Test 4: Set metatable
print("")
print("Test 4: Set metatable")
local success = setmetatable(obj, meta)
print("setmetatable returned: " .. tostring(success))

-- Test 5: Test getmetatable after setting
print("")
print("Test 5: getmetatable after setting")
local retrieved = getmetatable(obj)
if retrieved == nil then
    print("ERROR: getmetatable returned nil after setting!")
else
    print("getmetatable(obj) retrieved successfully")
    if retrieved == meta then
        print("SUCCESS: Retrieved metatable matches original")
    else
        print("WARNING: Retrieved metatable does not match original")
    end
end

print("")
print("=== Simple test completed ===")
