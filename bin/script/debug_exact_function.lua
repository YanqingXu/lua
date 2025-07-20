-- Test the exact function that was failing
print("=== Exact Function Debug ===")

function test_if_return(x)
    print("Inside test_if_return, x =", x)
    if x > 5 then
        print("x > 5, returning x")
        return x
    end
    print("x <= 5, returning 0")
    return 0
end

local result1 = test_if_return(10)
print("result1 =", result1)
print("Expected: 10")

local result2 = test_if_return(3)
print("result2 =", result2)
print("Expected: 0")

print("=== End Debug ===")
