-- Debug metamethods test
print("=== Debug Metamethods Test ===")

-- Test helper function
local function test_assert(condition, test_name)
    if condition then
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        print("[Failed] " .. test_name .. " - Failed")
        return false
    end
end

print("Testing __call metamethod...")
local callable_obj = {}
local callable_meta = {
    __call = function(self, x, y)
        return x + y + 100
    end
}
setmetatable(callable_obj, callable_meta)
local call_result = callable_obj(5, 3)
test_assert(call_result == 108, "__call metamethod")

print("Testing __eq metamethod...")
local eq_obj1 = {value = 10}
local eq_obj2 = {value = 10}
local eq_meta = {
    __eq = function(a, b)
        return a.value == b.value
    end
}
setmetatable(eq_obj1, eq_meta)
setmetatable(eq_obj2, eq_meta)
test_assert(eq_obj1 == eq_obj2, "__eq metamethod")

print("Testing __concat metamethod...")
local concat_obj1 = {text = "Hello"}
local concat_obj2 = {text = "World"}
local concat_meta = {
    __concat = function(a, b)
        return a.text .. " " .. b.text
    end
}
setmetatable(concat_obj1, concat_meta)
setmetatable(concat_obj2, concat_meta)
local concat_result = concat_obj1 .. concat_obj2
test_assert(concat_result == "Hello World", "__concat metamethod")

print("Testing arithmetic metamethods...")
local math_obj1 = {value = 10}
local math_obj2 = {value = 5}
local math_meta = {
    __add = function(a, b) return {value = a.value + b.value} end,
    __sub = function(a, b) return {value = a.value - b.value} end,
    __mul = function(a, b) return {value = a.value * b.value} end
}
setmetatable(math_obj1, math_meta)
setmetatable(math_obj2, math_meta)

local add_result = math_obj1 + math_obj2
local sub_result = math_obj1 - math_obj2
local mul_result = math_obj1 * math_obj2

test_assert(add_result.value == 15, "__add metamethod")
test_assert(sub_result.value == 5, "__sub metamethod")
test_assert(mul_result.value == 50, "__mul metamethod")

print("=== Metamethods test completed ===")
