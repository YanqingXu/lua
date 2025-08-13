-- Debug function test
print("Starting debug test")

local function test()
    print("Inside test function")
    return 123
end

print("About to call test")
local result = test()
print("test() returned:", result)
print("Debug test completed")
