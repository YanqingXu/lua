-- Debug different return types after function call
print("=== Return Types Debug ===")

-- Test 1: Return parameter (fails)
function test_return_param(x)
    if x > 5 then
        print("debug")
        return x
    end
    return 0
end

print("Return parameter:", test_return_param(10))

-- Test 2: Return local variable (works)
function test_return_local(x)
    local y = x
    if y > 5 then
        print("debug")
        return y
    end
    return 0
end

print("Return local:", test_return_local(10))

-- Test 3: Return constant (fails)
function test_return_constant(x)
    if x > 5 then
        print("debug")
        return 42
    end
    return 0
end

print("Return constant:", test_return_constant(10))

-- Test 4: Return arithmetic expression (fails?)
function test_return_arithmetic(x)
    if x > 5 then
        print("debug")
        return x + 1
    end
    return 0
end

print("Return arithmetic:", test_return_arithmetic(10))

print("=== End Debug ===")
