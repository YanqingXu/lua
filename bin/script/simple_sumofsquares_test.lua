-- Simple sumOfSquares test without context
function square(n)
    return n * n
end

function sumOfSquares(a, b)
    local x = square(a)
    local y = square(b)
    return x + y
end

local result = sumOfSquares(3, 4)
print("sumOfSquares(3, 4) =", result)
