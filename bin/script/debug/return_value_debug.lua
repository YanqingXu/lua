print("=== Return Value Debug ===")

-- Test 1: Simple return value
local function simple_return()
    print("  simple_return called")
    return 42
end

print("Test 1: Simple return")
local result1 = simple_return()
print("Result1:", result1, "type:", type(result1))

-- Test 2: Conditional return
local function conditional_return(flag)
    print("  conditional_return called with flag =", flag)
    if flag then
        print("  returning 100")
        return 100
    else
        print("  returning 200")
        return 200
    end
end

print("\nTest 2: Conditional return")
local result2 = conditional_return(true)
print("Result2:", result2, "type:", type(result2))

local result3 = conditional_return(false)
print("Result3:", result3, "type:", type(result3))

-- Test 3: Recursive base case
local function base_case(n)
    print("  base_case called with n =", n)
    if n <= 0 then
        print("  base case: returning 999")
        return 999
    else
        print("  not base case")
        return 888
    end
end

print("\nTest 3: Base case")
local result4 = base_case(0)
print("Result4:", result4, "type:", type(result4))

local result5 = base_case(1)
print("Result5:", result5, "type:", type(result5))

print("\n=== Return Value Debug Complete ===")
