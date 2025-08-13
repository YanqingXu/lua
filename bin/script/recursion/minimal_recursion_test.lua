print("=== Minimal Recursion Test ===")

-- Test: Minimal recursion case
local function test_rec(n)
    print("test_rec called with n =", n)
    if n <= 0 then
        return 0
    else
        return test_rec(n - 1)
    end
end

print("Calling test_rec(1)...")
local result = test_rec(1)
print("Result:", result)

print("=== Minimal Recursion Test Complete ===")
