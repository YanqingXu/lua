-- Function Features Test
-- Tests function definition, calls, and recursion

print("=== Function Features Test ===")

local test_count = 0
local passed_count = 0

function test_assert(condition, test_name)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name)
    else
        print("[FAILED] " .. test_name)
    end
end

-- Simple Function Test
print("Testing simple functions...")

function add(a, b)
    return a + b
end

local result1 = add(5, 3)
test_assert(result1 == 8, "Simple function call")

function multiply(x, y)
    return x * y
end

local result2 = multiply(4, 6)
test_assert(result2 == 24, "Function with different parameters")

-- Function with Local Variables
print("Testing functions with local variables...")

function calculate_area(width, height)
    local area = width * height
    return area
end

local area_result = calculate_area(5, 4)
test_assert(area_result == 20, "Function with local variables")

-- Recursive Function Test
print("Testing recursive functions...")

function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

local fact_result = factorial(5)
test_assert(fact_result == 120, "Recursive function (factorial)")

local fact_result2 = factorial(4)
test_assert(fact_result2 == 24, "Recursive function (factorial 4)")

-- Function as Variable
print("Testing functions as variables...")

local my_function = function(a, b)
    return a - b
end

local sub_result = my_function(10, 3)
test_assert(sub_result == 7, "Function assigned to variable")

-- Nested Function Calls
print("Testing nested function calls...")

function double(x)
    return x * 2
end

function triple(x)
    return x * 3
end

local nested_result = double(triple(4))
test_assert(nested_result == 24, "Nested function calls")

-- Function with Conditional Logic
print("Testing functions with conditional logic...")

function max(a, b)
    if a > b then
        return a
    else
        return b
    end
end

local max_result1 = max(10, 5)
test_assert(max_result1 == 10, "Function with conditional (first larger)")

local max_result2 = max(3, 8)
test_assert(max_result2 == 8, "Function with conditional (second larger)")

-- Summary
print("")
print("=== Function Features Test Summary ===")
print("Total tests: " .. tostring(test_count))
print("Passed tests: " .. tostring(passed_count))

local failed_count = test_count - passed_count
print("Failed tests: " .. tostring(failed_count))

if passed_count == test_count then
    print("All function features working correctly!")
else
    print("Some function features failed")
end

print("=== Function Features Test Completed ===")
