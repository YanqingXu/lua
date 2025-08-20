-- Simple nested function call test
function square(x)
    return x * x
end

function sumOfSquares(a, b)
    return square(a) + square(b)
end

local result = sumOfSquares(3, 4)
print("sumOfSquares(3, 4) =", result)
