-- Progressive test to isolate the context issue
print("=== Progressive Test ===")

-- Step 1: Basic variables
local a = 10
local b = 5
print("a =", a, "b =", b)

-- Step 2: Basic function
function multiply(x, y)
    return x * y
end

local result1 = multiply(6, 7)
print("multiply(6, 7) =", result1)

-- Step 3: Nested functions (this should fail in complex context)
function square(n)
    return n * n
end

function sumOfSquares(a, b)
    return square(a) + square(b)
end

local result2 = sumOfSquares(3, 4)
print("sumOfSquares(3, 4) =", result2)
