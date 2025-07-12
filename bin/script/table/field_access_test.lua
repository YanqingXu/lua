-- Simple field access test
print("=== Simple Field Access Test ===")

-- Test basic field access
local t = {}
print("Created empty table t")

t.x = 10
print("Set t.x = 10")
print("t.x = " .. tostring(t.x))

t.y = "hello"
print("Set t.y = 'hello'")
print("t.y = " .. tostring(t.y))

-- Test field access patterns
local obj = {existing = "value"}
print("Created obj = {existing = 'value'}")
print("obj.existing = " .. tostring(obj.existing))

-- Test with local variable
local val = obj.existing
print("val = obj.existing")
print("val = " .. tostring(val))

print("=== Field access test completed ===")
