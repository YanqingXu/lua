print("=== Simple Function Debug ===")

print("Before function definition")

local function test_func()
    print("Inside test_func")
    return 42
end

print("After function definition")

print("Before function call")
local result = test_func()
print("After function call")

print("Result:", result)

print("=== Simple Function Debug Complete ===")
