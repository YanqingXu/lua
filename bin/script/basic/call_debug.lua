print("=== Call Debug ===")

-- Test: Check function object before and after parameter
local function test_func(x)
    print("  test_func called with x =", x)
    return x + 1
end

print("Function defined")
print("test_func type:", type(test_func))

-- Test with literal parameter
print("\nCalling with literal 5...")
local result = test_func(5)
print("Result:", result)

print("\n=== Call Debug Complete ===")
