-- Debug function return values
print("=== Debug Function Return ===")

-- Test 1: Simple function with return
function add_numbers(a, b)
    print("Inside add_numbers: a =", a, "b =", b)
    local result = a + b
    print("Inside add_numbers: result =", result)
    return result
end

local sum = add_numbers(3, 4)
print("sum =", sum)
print("Expected: 7")

-- Test 2: Function without explicit return
function print_hello()
    print("Hello from function!")
end

local hello_result = print_hello()
print("hello_result =", hello_result)
print("Expected: nil")

-- Test 3: Function with conditional return
function max_value(a, b)
    print("Inside max_value: a =", a, "b =", b)
    if a > b then
        print("Returning a:", a)
        return a
    else
        print("Returning b:", b)
        return b
    end
end

local max_result = max_value(5, 3)
print("max_result =", max_result)
print("Expected: 5")

print("=== End ===")
