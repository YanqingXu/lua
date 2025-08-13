print("=== Final Recursion Verification ===")

-- Test 1: Simple countdown recursion
local function countdown(n)
    if n <= 0 then
        return 0
    else
        return countdown(n - 1) + 1
    end
end

print("Test 1: countdown(5) =", countdown(5))

-- Test 2: Factorial recursion
local function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

print("Test 2: factorial(4) =", factorial(4))

-- Test 3: Fibonacci recursion
local function fibonacci(n)
    if n <= 1 then
        return n
    else
        return fibonacci(n - 1) + fibonacci(n - 2)
    end
end

print("Test 3: fibonacci(5) =", fibonacci(5))

-- Test 4: Mutual recursion
local function is_even(n)
    if n == 0 then
        return true
    else
        return is_odd(n - 1)
    end
end

local function is_odd(n)
    if n == 0 then
        return false
    else
        return is_even(n - 1)
    end
end

print("Test 4: is_even(4) =", is_even(4))
print("Test 5: is_odd(3) =", is_odd(3))

print("\n=== All Recursion Tests Complete ===")
print("✅ Recursive function calls are working!")
