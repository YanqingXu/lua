print("=== Detailed Recursion Test ===")

-- Test: Step by step recursion analysis
local function countdown(n)
    print("  countdown called with n =", n)
    print("  countdown type inside function:", type(countdown))
    
    if n <= 0 then
        print("  base case: returning 0")
        return 0
    else
        print("  recursive case: about to call countdown(" .. tostring(n-1) .. ")")
        local result = countdown(n - 1)
        print("  recursive call returned:", result)
        return result + 1
    end
end

print("Function defined")
print("countdown type:", type(countdown))

print("\nCalling countdown(2)...")
local final_result = countdown(2)
print("Final result:", final_result)

print("\n=== Detailed Recursion Test Complete ===")
