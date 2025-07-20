-- Metatable Features Test
-- Tests metatable operations and metamethods

print("=== Metatable Features Test ===")

local test_count = 0
local passed_count = 0

function test_assert(condition, test_name)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name)
    else
        print("[FAILED] " .. test_name)
    end
end

-- Basic Metatable Operations
print("Testing basic metatable operations...")

local obj = {}
local mt = {}
setmetatable(obj, mt)
local retrieved_mt = getmetatable(obj)
test_assert(retrieved_mt == mt, "Metatable set and get")

-- __index Metamethod
print("Testing __index metamethod...")

local defaults = {default_value = "default", number_value = 42}
mt.__index = defaults
local default_val = obj.default_value
local number_val = obj.number_value
test_assert(default_val == "default", "__index metamethod (string)")
test_assert(number_val == 42, "__index metamethod (number)")

-- __newindex Metamethod
print("Testing __newindex metamethod...")

local storage = {}
mt.__newindex = storage
obj.new_field = "new_value"
obj.another_field = 123
local stored_val1 = storage.new_field
local stored_val2 = storage.another_field
test_assert(stored_val1 == "new_value", "__newindex metamethod (string)")
test_assert(stored_val2 == 123, "__newindex metamethod (number)")

-- __tostring Metamethod
print("Testing __tostring metamethod...")

local ts_obj = {}
local ts_mt = {
    __tostring = function(o)
        return "custom_string_representation"
    end
}
setmetatable(ts_obj, ts_mt)
local tostring_result = tostring(ts_obj)
test_assert(tostring_result == "custom_string_representation", "__tostring metamethod")

-- __eq Metamethod
print("Testing __eq metamethod...")

local eq_obj1 = {value = 10}
local eq_obj2 = {value = 10}
local eq_obj3 = {value = 20}
local eq_mt = {
    __eq = function(a, b)
        return a.value == b.value
    end
}
setmetatable(eq_obj1, eq_mt)
setmetatable(eq_obj2, eq_mt)
setmetatable(eq_obj3, eq_mt)

local eq_result1 = eq_obj1 == eq_obj2
local eq_result2 = eq_obj1 == eq_obj3
test_assert(eq_result1 == true, "__eq metamethod (equal values)")
test_assert(eq_result2 == false, "__eq metamethod (different values)")

-- __add Metamethod
print("Testing __add metamethod...")

local add_obj1 = {value = 10}
local add_obj2 = {value = 5}
local add_mt = {
    __add = function(a, b)
        return {value = a.value + b.value}
    end
}
setmetatable(add_obj1, add_mt)
setmetatable(add_obj2, add_mt)

local add_result = add_obj1 + add_obj2
test_assert(add_result.value == 15, "__add metamethod")

-- __sub Metamethod
print("Testing __sub metamethod...")

local sub_obj1 = {value = 10}
local sub_obj2 = {value = 3}
local sub_mt = {
    __sub = function(a, b)
        return {value = a.value - b.value}
    end
}
setmetatable(sub_obj1, sub_mt)
setmetatable(sub_obj2, sub_mt)

local sub_result = sub_obj1 - sub_obj2
test_assert(sub_result.value == 7, "__sub metamethod")

-- __mul Metamethod
print("Testing __mul metamethod...")

local mul_obj1 = {value = 6}
local mul_obj2 = {value = 4}
local mul_mt = {
    __mul = function(a, b)
        return {value = a.value * b.value}
    end
}
setmetatable(mul_obj1, mul_mt)
setmetatable(mul_obj2, mul_mt)

local mul_result = mul_obj1 * mul_obj2
test_assert(mul_result.value == 24, "__mul metamethod")

-- __concat Metamethod
print("Testing __concat metamethod...")

local concat_obj1 = {text = "Hello"}
local concat_obj2 = {text = "World"}
local concat_mt = {
    __concat = function(a, b)
        return a.text .. " " .. b.text
    end
}
setmetatable(concat_obj1, concat_mt)
setmetatable(concat_obj2, concat_mt)

local concat_result = concat_obj1 .. concat_obj2
test_assert(concat_result == "Hello World", "__concat metamethod")

-- Summary
print("")
print("=== Metatable Features Test Summary ===")
print("Total tests: " .. tostring(test_count))
print("Passed tests: " .. tostring(passed_count))

local failed_count = test_count - passed_count
print("Failed tests: " .. tostring(failed_count))

if passed_count == test_count then
    print("All metatable features working correctly!")
else
    print("Some metatable features failed")
end

print("=== Metatable Features Test Completed ===")
