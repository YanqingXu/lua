-- Advanced metamethod test
print("=== Advanced Metamethod Test ===")

-- Test 1: __index metamethod with table
print("Test 1: __index metamethod with table")
local t = {}
local mt = {}
local fallback = {default_value = "from fallback table"}

mt.__index = fallback
setmetatable(t, mt)

print("t.default_value = " .. tostring(t.default_value))
print("t.nonexistent = " .. tostring(t.nonexistent))

-- Test 2: __newindex metamethod with table
print("")
print("Test 2: __newindex metamethod with table")
local t2 = {}
local mt2 = {}
local storage = {}

mt2.__newindex = storage
setmetatable(t2, mt2)

print("Setting t2.new_key = 'new_value'")
t2.new_key = "new_value"
print("t2.new_key = " .. tostring(t2.new_key))
print("storage.new_key = " .. tostring(storage.new_key))

-- Test 3: __tostring metamethod
print("")
print("Test 3: __tostring metamethod")
local t3 = {name = "MyObject"}
local mt3 = {}

-- Note: __tostring with function requires function call support
-- For now, we'll test the basic structure
mt3.__tostring = "custom string representation"
setmetatable(t3, mt3)

print("Created object with __tostring metamethod")
print("Object: " .. tostring(t3))

-- Test 4: Arithmetic metamethods (basic structure)
print("")
print("Test 4: Arithmetic metamethods structure")
local t4 = {value = 10}
local mt4 = {}

-- Note: These require function call support to work fully
mt4.__add = "addition metamethod"
mt4.__sub = "subtraction metamethod"
mt4.__mul = "multiplication metamethod"

setmetatable(t4, mt4)
print("Created object with arithmetic metamethods")

print("")
print("=== Advanced metamethod test completed ===")
print("Note: Function-based metamethods require full function call support")
