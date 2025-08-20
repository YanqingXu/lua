-- Test sumOfSquares function specifically
function square(n)
    return n * n
end

function sumOfSquares(a, b)
    return square(a) + square(b)
end

local result = sumOfSquares(3, 4)
print("sumOfSquares(3, 4) =", result)
