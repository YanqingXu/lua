print("=== Optimized Function Call Test ===")

-- Test 1: Simple function call
print("\nTest 1: Simple function call")
local function add(a, b)
    print("  Inside add function: a =", a, "b =", b)
    return a + b
end

local result = add(3, 5)
print("  Result:", result)

-- Test 2: Function with local variables
print("\nTest 2: Function with local variables")
local function calculate(x)
    local temp = x * 2
    local result = temp + 10
    print("  Inside calculate: x =", x, "temp =", temp, "result =", result)
    return result
end

local calc_result = calculate(7)
print("  Calculate result:", calc_result)

-- Test 3: Nested function calls
print("\nTest 3: Nested function calls")
local function multiply(a, b)
    print("  Inside multiply: a =", a, "b =", b)
    return a * b
end

local function complex_calc(x, y)
    print("  Inside complex_calc: x =", x, "y =", y)
    local sum = add(x, y)
    local product = multiply(x, y)
    print("  sum =", sum, "product =", product)
    return sum + product
end

local complex_result = complex_calc(4, 6)
print("  Complex result:", complex_result)

print("\n=== Optimized Function Call Test Complete ===")
