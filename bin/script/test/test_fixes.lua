-- Test fixes for metamethod issues
print("=== Testing Fixes ===")

-- Test 1: Table literal initialization
print("Test 1: Table literal initialization")
local t1 = {name = "test", value = 42}
print("Created table with literal")

if t1.name == nil then
    print("ERROR: t1.name is nil!")
else
    print("SUCCESS: t1.name = " .. t1.name)
end

if t1.value == nil then
    print("ERROR: t1.value is nil!")
else
    print("SUCCESS: t1.value = " .. tostring(t1.value))
end

-- Test 2: String concatenation
print("")
print("Test 2: String concatenation")
local str1 = "Hello"
local str2 = "World"
local result = str1 .. " " .. str2
print("Concatenation result: " .. result)

-- Test 3: Metatable identity
print("")
print("Test 3: Metatable identity")
local t2 = {}
local mt = {type = "test"}
setmetatable(t2, mt)
local retrieved_mt = getmetatable(t2)

if mt == retrieved_mt then
    print("SUCCESS: Metatable identity preserved")
else
    print("ERROR: Metatable identity lost")
end

print("")
print("=== Test completed ===")
