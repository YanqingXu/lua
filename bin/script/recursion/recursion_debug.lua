print("=== Recursion Debug ===")

-- Test 1: Check if function can access itself
print("\nTest 1: Function self-access")
local function test_self()
    print("  Inside test_self")
    print("  test_self type:", type(test_self))
    return 42
end

print("Before call: test_self type:", type(test_self))
local result1 = test_self()
print("Result1:", result1)

-- Test 2: Simple conditional recursion
print("\nTest 2: Conditional recursion")
local function simple_rec(n)
    print("  simple_rec called with n =", n)
    print("  simple_rec type inside:", type(simple_rec))
    
    if n <= 0 then
        print("  Base case: returning 0")
        return 0
    else
        print("  Recursive case: calling simple_rec(" .. tostring(n-1) .. ")")
        local result = simple_rec(n - 1)
        print("  Got result from recursive call:", result)
        return result + 1
    end
end

print("Before call: simple_rec type:", type(simple_rec))
local result2 = simple_rec(2)
print("Final result2:", result2)

print("\n=== Recursion Debug Complete ===")
