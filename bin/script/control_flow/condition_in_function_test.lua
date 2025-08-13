print("=== Condition in Function Test ===")

-- Test: Condition inside function
local function test_condition(n)
    print("  test_condition called with n =", n)
    
    if n <= 0 then
        print("  condition is true")
        return 100
    else
        print("  condition is false")
        return 200
    end
end

print("Testing with n = 0...")
local result1 = test_condition(0)
print("Result1:", result1)

print("\nTesting with n = 5...")
local result2 = test_condition(5)
print("Result2:", result2)

print("\n=== Condition in Function Test Complete ===")
