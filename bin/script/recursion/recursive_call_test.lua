print("=== Recursive Call Test ===")

-- Test 1: Simple recursion (factorial)
print("\nTest 1: Factorial recursion")
local function factorial(n)
    print("  factorial(" .. tostring(n) .. ")")
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

local fact_result = factorial(5)
print("  Factorial(5) =", fact_result)

-- Test 2: Fibonacci recursion (limited depth)
print("\nTest 2: Fibonacci recursion")
local function fibonacci(n)
    print("  fibonacci(" .. tostring(n) .. ")")
    if n <= 1 then
        return n
    else
        return fibonacci(n - 1) + fibonacci(n - 2)
    end
end

local fib_result = fibonacci(6)
print("  Fibonacci(6) =", fib_result)

-- Test 3: Mutual recursion
print("\nTest 3: Mutual recursion")
local function is_even(n)
    print("  is_even(" .. tostring(n) .. ")")
    if n == 0 then
        return true
    else
        return is_odd(n - 1)
    end
end

local function is_odd(n)
    print("  is_odd(" .. tostring(n) .. ")")
    if n == 0 then
        return false
    else
        return is_even(n - 1)
    end
end

local even_result = is_even(4)
print("  is_even(4) =", even_result)

print("\n=== Recursive Call Test Complete ===")
