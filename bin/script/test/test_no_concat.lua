-- Test without string concatenation
print("=== Test Without String Concatenation ===")

-- Test 1: Basic metatable operations
print("Test 1: Basic metatable operations")
local obj = {}
obj.name = "test"

local meta = {}
meta.type = "metatable"

print("Before setmetatable:")
print("obj.name:", obj.name)
print("meta.type:", meta.type)

setmetatable(obj, meta)
print("setmetatable completed")

local retrieved = getmetatable(obj)
print("getmetatable completed")

if retrieved then
    print("Retrieved metatable: table")
    if retrieved.type then
        print("retrieved.type:", retrieved.type)
    else
        print("retrieved.type: nil")
    end
else
    print("Retrieved metatable: nil")
end

-- Test 2: __index test
print("\nTest 2: __index test")
local storage = {}
storage.default_value = "from_storage"
meta.__index = storage

print("Set __index to storage")
print("storage.default_value:", storage.default_value)

local result = obj.default_value
print("obj.default_value:", tostring(result))

-- Test 3: __newindex test
print("\nTest 3: __newindex test")
local new_storage = {}
meta.__newindex = new_storage

print("Set __newindex to new_storage")
obj.new_field = "new_value"
print("Set obj.new_field to new_value")

if new_storage.new_field then
    print("new_storage.new_field:", new_storage.new_field)
else
    print("new_storage.new_field: nil")
end

print("\n=== Test completed ===")
