print("=== Return Value Test ===")

-- Test 1: Function returning a number
local function return_number()
    return 42
end

local num = return_number()
print("num =", num)

-- Test 2: Function returning a string
local function return_string()
    return "hello"
end

local str = return_string()
print("str =", str)

print("=== Return Value Test Complete ===")
