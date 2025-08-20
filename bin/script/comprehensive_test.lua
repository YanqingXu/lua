-- Comprehensive test: verify all functions
print("=== Lua Interpreter Comprehensive Test ===")

-- 1. Basic arithmetic operations
print("\n1. Basic arithmetic operations:")
local a = 10
local b = 5
print("a =", a)
print("b =", b)
print("a + b =", a + b)
print("a - b =", a - b)
print("a * b =", a * b)
print("a / b =", a / b)

-- 2. Conditional statements
print("\n2. Conditional statements:")
if a > b then
    print("a is greater than b")
else
    print("a is not greater than b")
end

-- 3. for loop
print("\n3. for loop:")
local sum = 0
for i = 1, 5 do
    sum = sum + i
    print("i =", i, "sum =", sum)
end
print("final sum =", sum)

-- 4. Function definition and call
print("\n4. Function definition and call:")
function multiply(x, y)
    return x * y
end

local result = multiply(6, 7)
print("multiply(6, 7) =", result)

-- 5. Nested function calls
print("\n5. Nested function calls:")
function square(n)
    return n * n
end

function sumOfSquares(a, b)
    return square(a) + square(b)
end

local nested_result = sumOfSquares(3, 4)
print("sumOfSquares(3, 4) =", nested_result)

print("\n=== All tests completed ===")
