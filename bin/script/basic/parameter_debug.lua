print("=== Parameter Debug ===")

-- Test 1: No parameters
local function no_params()
    print("  no_params called")
    return 42
end

print("Test 1: No parameters")
local result1 = no_params()
print("Result1:", result1)

-- Test 2: One parameter
local function one_param(x)
    print("  one_param called with x =", x)
    return x + 10
end

print("\nTest 2: One parameter")
local result2 = one_param(5)
print("Result2:", result2)

print("\n=== Parameter Debug Complete ===")
