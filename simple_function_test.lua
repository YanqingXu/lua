-- Very simple function test
print("=== Simple Function Test ===")

-- Test basic function call
local function test()
    print("Inside function")
    return 123
end

print("Before function call")
local result = test()
print("After function call")
print("Result:", result)

print("=== Test completed ===")
