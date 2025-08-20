-- Debug nested function call issue
function square(n)
    return n * n
end

function sumOfSquares(a, b)
    return square(a) + square(b)
end

local result = sumOfSquares(3, 4)
print("result =", result)
