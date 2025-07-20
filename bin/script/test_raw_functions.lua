-- Raw Table Functions Test
-- Tests rawget, rawset, rawlen, rawequal functions

print("=== Raw Table Functions Test ===")

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

-- rawget Function
print("Testing rawget function...")

local table1 = {existing = "value", number_key = 42}
local result1 = rawget(table1, "existing")
test_assert(result1 == "value", "rawget existing key (string)")

local result2 = rawget(table1, "number_key")
test_assert(result2 == 42, "rawget existing key (number value)")

local result3 = rawget(table1, "nonexistent")
test_assert(result3 == nil, "rawget nonexistent key")

-- rawget with Metatable
print("Testing rawget with metatable...")

local table2 = {real_key = "real_value"}
local mt = {
    __index = function(t, k)
        return "meta_value"
    end
}
setmetatable(table2, mt)

local meta_access = table2.nonexistent_key
test_assert(meta_access == "meta_value", "Normal access uses metatable")

local raw_access = rawget(table2, "nonexistent_key")
test_assert(raw_access == nil, "rawget bypasses metatable")

local raw_real = rawget(table2, "real_key")
test_assert(raw_real == "real_value", "rawget gets real value")

-- rawset Function
print("Testing rawset function...")

local table3 = {}
rawset(table3, "new_key", "new_value")
local set_result1 = rawget(table3, "new_key")
test_assert(set_result1 == "new_value", "rawset string key")

rawset(table3, 100, "numeric_key_value")
local set_result2 = rawget(table3, 100)
test_assert(set_result2 == "numeric_key_value", "rawset numeric key")

rawset(table3, "overwrite", "original")
rawset(table3, "overwrite", "modified")
local set_result3 = rawget(table3, "overwrite")
test_assert(set_result3 == "modified", "rawset overwrite value")

-- rawset with Metatable
print("Testing rawset with metatable...")

local table4 = {}
local storage = {}
local mt2 = {
    __newindex = function(t, k, v)
        storage[k] = v
    end
}
setmetatable(table4, mt2)

table4.meta_key = "meta_value"
local meta_stored = storage.meta_key
test_assert(meta_stored == "meta_value", "Normal assignment uses metatable")

rawset(table4, "raw_key", "raw_value")
local raw_stored = rawget(table4, "raw_key")
local meta_check = storage.raw_key
test_assert(raw_stored == "raw_value", "rawset bypasses metatable")
test_assert(meta_check == nil, "rawset does not trigger __newindex")

-- rawlen Function
print("Testing rawlen function...")

local array1 = {1, 2, 3, 4, 5}
local len1 = rawlen(array1)
test_assert(len1 == 5, "rawlen array length")

local array2 = {10, 20, 30}
local len2 = rawlen(array2)
test_assert(len2 == 3, "rawlen different array")

local string1 = "hello"
local len3 = rawlen(string1)
test_assert(len3 == 5, "rawlen string length")

local string2 = "world"
local len4 = rawlen(string2)
test_assert(len4 == 5, "rawlen different string")

local empty_string = ""
local len5 = rawlen(empty_string)
test_assert(len5 == 0, "rawlen empty string")

-- rawequal Function
print("Testing rawequal function...")

local obj1 = {}
local obj2 = {}
local obj3 = obj1

local equal1 = rawequal(obj1, obj1)
test_assert(equal1 == true, "rawequal same object")

local equal2 = rawequal(obj1, obj2)
test_assert(equal2 == false, "rawequal different objects")

local equal3 = rawequal(obj1, obj3)
test_assert(equal3 == true, "rawequal same reference")

local equal4 = rawequal(5, 5)
test_assert(equal4 == true, "rawequal same numbers")

local equal5 = rawequal(5, 10)
test_assert(equal5 == false, "rawequal different numbers")

local equal6 = rawequal("hello", "hello")
test_assert(equal6 == true, "rawequal same strings")

local equal7 = rawequal("hello", "world")
test_assert(equal7 == false, "rawequal different strings")

-- rawequal with Metatable
print("Testing rawequal with metatable...")

local eq_obj1 = {value = 10}
local eq_obj2 = {value = 10}
local eq_mt = {
    __eq = function(a, b)
        return a.value == b.value
    end
}
setmetatable(eq_obj1, eq_mt)
setmetatable(eq_obj2, eq_mt)

local meta_equal = eq_obj1 == eq_obj2
local raw_equal = rawequal(eq_obj1, eq_obj2)
test_assert(meta_equal == true, "Metatable equality works")
test_assert(raw_equal == false, "rawequal ignores metatable")

-- Summary
print("")
print("=== Raw Table Functions Test Summary ===")
print("Total tests: " .. tostring(test_count))
print("Passed tests: " .. tostring(passed_count))

local failed_count = test_count - passed_count
print("Failed tests: " .. tostring(failed_count))

if passed_count == test_count then
    print("All raw table functions working correctly!")
else
    print("Some raw table functions failed")
end

print("=== Raw Table Functions Test Completed ===")
