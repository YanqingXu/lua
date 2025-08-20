-- Test to reproduce the context issue
print("=== Context Test ===")

-- Simulate some previous operations to affect VM state
local a = 10
local b = 5
print("a =", a)
print("b =", b)

-- Basic function call (this works)
function multiply(x, y)
    return x * y
end

local result1 = multiply(6, 7)
print("multiply(6, 7) =", result1)

-- Now test nested function calls
function square(n)
    return n * n
end

function sumOfSquares(a, b)
    return square(a) + square(b)
end

local result2 = sumOfSquares(3, 4)
print("sumOfSquares(3, 4) =", result2)
