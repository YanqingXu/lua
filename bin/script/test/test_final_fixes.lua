-- Test final fixes for metamethod issues
print("=== Testing Final Fixes ===")

-- Test 1: Basic table literal
print("Test 1: Basic table literal")
local t1 = {a = 1, b = 2}
print("t1.a = " .. tostring(t1.a))
print("t1.b = " .. tostring(t1.b))

-- Test 2: String concatenation
print("")
print("Test 2: String concatenation")
local str = "Hello" .. " " .. "World"
print("Concatenation: " .. str)

-- Test 3: Metatable identity
print("")
print("Test 3: Metatable identity")
local t2 = {}
local mt = {type = "test"}
setmetatable(t2, mt)
local retrieved_mt = getmetatable(t2)
print("Metatable identity: " .. tostring(mt == retrieved_mt))

-- Test 4: Basic metamethod (__index)
print("")
print("Test 4: __index metamethod")
local t3 = {}
local mt3 = {
    __index = function(table, key)
        print("__index called for key: " .. tostring(key))
        return "metamethod_result"
    end
}
setmetatable(t3, mt3)
local result = t3.nonexistent
print("Result: " .. tostring(result))

print("")
print("=== Test completed ===")
