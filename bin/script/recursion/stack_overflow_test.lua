print("=== Stack Overflow Test ===")

-- Test: Deep recursion to trigger stack overflow protection
print("\nTest: Deep recursion")

local function deep_recursion(n)
    if n <= 0 then
        return 0
    else
        return 1 + deep_recursion(n - 1)
    end
end

print("Testing deep recursion with n = 10...")
local result = deep_recursion(10)
print("Result:", result)

print("Testing deep recursion with n = 100...")
local result2 = deep_recursion(100)
print("Result:", result2)

print("\n=== Stack Overflow Test Complete ===")
