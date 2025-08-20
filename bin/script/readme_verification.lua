-- README Documentation Verification Test
print("=== README Documentation Verification ===")

-- Test all features mentioned in the updated README

-- 1. Basic arithmetic operations
local a = 10
local b = 5
print("1. Basic arithmetic:")
print("   a + b =", a + b)  -- Should output 15
print("   a * b =", a * b)  -- Should output 50

-- 2. Conditional statements
if a > b then
    print("2. Conditional: a is greater than b")  -- Should execute
end

-- 3. For loops
print("3. For loop:")
local sum = 0
for i = 1, 3 do
    sum = sum + i
    print("   i =", i, "sum =", sum)
end

-- 4. Function definition and calls
function multiply(x, y)
    return x * y
end
print("4. Function call: multiply(6, 7) =", multiply(6, 7))  -- Should output 42

-- 5. Nested function calls (THE NEW ACHIEVEMENT!)
function square(n)
    return n * n
end

function sumOfSquares(a, b)
    return square(a) + square(b)
end

local result = sumOfSquares(3, 4)
print("5. Nested function call: sumOfSquares(3, 4) =", result)  -- Should output 25

print("=== All README features verified ===")
