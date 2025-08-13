-- Function call test
print("=== Function Call Test ===")

local function simple()
    print("Simple function called")
    return 42
end

print("About to call simple()")
local result = simple()
print("simple() returned:", result)

local function add(a, b)
    print("add function called with", a, "and", b)
    return a + b
end

print("About to call add(10, 20)")
local sum = add(10, 20)
print("add(10, 20) returned:", sum)

print("=== Test completed ===")
