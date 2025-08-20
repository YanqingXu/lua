-- Test direct nested function calls
print("=== Nested Call Debug Test ===")

function multiply(x, y)
    return x * y
end

-- This should work but currently fails
print("Direct nested call:", multiply(6, 7))

print("=== Test completed ===")
