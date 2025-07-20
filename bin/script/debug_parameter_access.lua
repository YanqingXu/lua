-- Debug parameter access after function call
print("=== Parameter Access Debug ===")

-- Test 1: Access parameter before function call
function test_param_before(x)
    local temp = x  -- Access parameter before call
    if temp > 5 then
        print("debug")
        return temp
    end
    return 0
end

print("Parameter before call:", test_param_before(10))

-- Test 2: Access parameter after function call
function test_param_after(x)
    if x > 5 then
        print("debug")
        local temp = x  -- Access parameter after call
        return temp
    end
    return 0
end

print("Parameter after call:", test_param_after(10))

-- Test 3: Direct parameter access after call
function test_direct_param(x)
    if x > 5 then
        print("debug")
        return x  -- Direct parameter access
    end
    return 0
end

print("Direct parameter access:", test_direct_param(10))

-- Test 4: Multiple parameters
function test_multiple_params(x, y)
    if x > 5 then
        print("debug")
        return y  -- Return second parameter
    end
    return 0
end

print("Multiple parameters:", test_multiple_params(10, 20))

print("=== End Debug ===")
