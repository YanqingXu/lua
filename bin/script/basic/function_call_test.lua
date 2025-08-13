print("=== Function Call Test ===")

-- Test 1: Simple function call
local function test()
    print("Inside test function")
    return 42
end

print("About to call test()")
local result = test()
print("test() returned:", result)

print("=== Function Call Test Complete ===")
