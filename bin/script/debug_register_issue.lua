-- Debug register allocation issue
print("=== Register Issue Debug ===")

-- Test: Store value in local variable before function call
function test_local_before_call(x)
    local temp = x  -- Store x in local variable
    if temp > 5 then
        print("About to return temp")
        return temp  -- Return local variable instead of parameter
    end
    return 0
end

print("Local before call test:", test_local_before_call(10))

-- Test: Store value in local variable after function call
function test_local_after_call(x)
    if x > 5 then
        print("About to store x")
        local temp = x  -- Store x after function call
        return temp
    end
    return 0
end

print("Local after call test:", test_local_after_call(10))

-- Test: Return constant after function call
function test_constant_after_call(x)
    if x > 5 then
        print("About to return constant")
        return 42  -- Return constant instead of variable
    end
    return 0
end

print("Constant after call test:", test_constant_after_call(10))

print("=== End Debug ===")
