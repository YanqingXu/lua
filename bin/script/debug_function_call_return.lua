-- Precise test for function call + return issue
print("=== Function Call Return Issue Debug ===")

-- Test 1: Simple return (works)
function simple_return(x)
    if x > 5 then
        return x
    end
    return 0
end

print("Simple return test:", simple_return(10))

-- Test 2: Return after function call (fails)
function return_after_call(x)
    if x > 5 then
        print("About to return x")
        return x
    end
    return 0
end

print("Return after call test:", return_after_call(10))

-- Test 3: Multiple function calls
function multiple_calls(x)
    if x > 5 then
        print("First call")
        print("Second call")
        return x
    end
    return 0
end

print("Multiple calls test:", multiple_calls(10))

-- Test 4: Function call in else branch
function call_in_else(x)
    if x > 5 then
        return x
    else
        print("In else branch")
        return 0
    end
end

print("Call in else test:", call_in_else(3))

print("=== End Debug ===")
