-- Test if print statements affect return
print("=== Print Effect Debug ===")

-- Test 1: Without print statements
function test_no_print(x)
    if x > 5 then
        return x
    end
    return 0
end

print("Test without print:", test_no_print(10))

-- Test 2: With print statements
function test_with_print(x)
    print("Inside function, x =", x)
    if x > 5 then
        print("Returning x")
        return x
    end
    print("Returning 0")
    return 0
end

print("Test with print:", test_with_print(10))

-- Test 3: Only print before return
function test_print_before_return(x)
    if x > 5 then
        print("About to return x")
        return x
    end
    return 0
end

print("Test print before return:", test_print_before_return(10))

print("=== End Debug ===")
