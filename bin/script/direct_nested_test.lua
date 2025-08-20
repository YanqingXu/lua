-- Test the original problematic pattern
function multiply(x, y)
    return x * y
end

-- This is the pattern that was failing before
print("Direct nested call:", multiply(6, 7))
