print("=== Simple Recursion Test ===")

-- Test: Very simple recursion
local function countdown(n)
    print("countdown(" .. tostring(n) .. ")")
    if n <= 0 then
        print("  Base case reached")
        return 0
    else
        print("  Recursive call")
        return countdown(n - 1)
    end
end

print("Testing countdown(3)...")
local result = countdown(3)
print("Result:", result)

print("=== Simple Recursion Test Complete ===")
