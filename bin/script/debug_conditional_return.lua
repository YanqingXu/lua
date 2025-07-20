-- Debug conditional return
print("=== Debug Conditional Return ===")

-- Test 1: Simple if-return
function test_if_return(x)
    print("Inside test_if_return, x =", x)
    if x > 5 then
        print("x > 5, returning x")
        return x
    end
    print("x <= 5, returning 0")
    return 0
end

local result1 = test_if_return(10)
print("result1 =", result1)
print("Expected: 10")

local result2 = test_if_return(3)
print("result2 =", result2)
print("Expected: 0")

-- Test 2: If-else return
function test_if_else_return(x)
    print("Inside test_if_else_return, x =", x)
    if x > 5 then
        print("x > 5, returning x * 2")
        return x * 2
    else
        print("x <= 5, returning x + 10")
        return x + 10
    end
end

local result3 = test_if_else_return(8)
print("result3 =", result3)
print("Expected: 16")

local result4 = test_if_else_return(2)
print("result4 =", result4)
print("Expected: 12")

print("=== End ===")
