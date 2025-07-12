-- Test access permission fix
print("=== Testing Access Permission Fix ===")

-- Test 1: Basic table literal (should work now)
print("Test 1: Basic table literal")
local t1 = {a = 1, b = 2}
print("t1.a = " .. tostring(t1.a))
print("t1.b = " .. tostring(t1.b))

-- Test 2: __index metamethod
print("")
print("Test 2: __index metamethod")
local t2 = {}
local mt2 = {
    __index = function(table, key)
        print("__index called for key: " .. tostring(key))
        return "from_metamethod"
    end
}
setmetatable(t2, mt2)
local result = t2.test_key
print("Result: " .. tostring(result))

-- Test 3: __newindex metamethod
print("")
print("Test 3: __newindex metamethod")
local t3 = {}
local mt3 = {
    __newindex = function(table, key, value)
        print("__newindex called: " .. tostring(key) .. " = " .. tostring(value))
        rawset(table, "intercepted_" .. key, value)
    end
}
setmetatable(t3, mt3)
t3.new_key = "test_value"
print("t3.new_key = " .. tostring(t3.new_key))
print("t3.intercepted_new_key = " .. tostring(t3.intercepted_new_key))

print("")
print("=== Test completed ===")
