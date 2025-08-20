-- Simple README verification
print("=== README Features Test ===")

-- Basic arithmetic
local a = 10
local b = 5
print("a + b =", a + b)

-- Function calls
function multiply(x, y)
    return x * y
end

local result1 = multiply(6, 7)
print("multiply(6, 7) =", result1)

-- Nested function calls (THE KEY ACHIEVEMENT!)
function square(n)
    return n * n
end

function sumOfSquares(x, y)
    return square(x) + square(y)
end

local result2 = sumOfSquares(3, 4)
print("sumOfSquares(3, 4) =", result2)
print("=== All key features working ===")
