-- Debug test for direct parameter return issue
print("=== Parameter Return Debug Test ===")

-- Test 1: Simple parameter return (should work)
function simple_param_return(x)
    return x
end

print("Test 1: Simple parameter return")
local result1 = simple_param_return(42)
print("simple_param_return(42) =", result1)
print("Expected: 42, Actual:", result1)
print("Status:", result1 == 42 and "✅ PASS" or "❌ FAIL")
print("")

-- Test 2: Parameter return after function call (fails)
function param_return_after_call(x)
    print("About to return parameter x")
    return x
end

print("Test 2: Parameter return after function call")
local result2 = param_return_after_call(100)
print("param_return_after_call(100) =", result2)
print("Expected: 100, Actual:", result2)
print("Status:", result2 == 100 and "✅ PASS" or "❌ FAIL")
print("")

-- Test 3: Multiple parameters
function multi_param_return(a, b, c)
    print("Parameters received:", a, b, c)
    return b  -- Return second parameter
end

print("Test 3: Multiple parameter return")
local result3 = multi_param_return(10, 20, 30)
print("multi_param_return(10, 20, 30) =", result3)
print("Expected: 20, Actual:", result3)
print("Status:", result3 == 20 and "✅ PASS" or "❌ FAIL")
print("")

-- Test 4: Parameter return in conditional
function conditional_param_return(x)
    if x > 50 then
        print("x is greater than 50")
        return x
    else
        print("x is not greater than 50")
        return 0
    end
end

print("Test 4: Conditional parameter return")
local result4 = conditional_param_return(75)
print("conditional_param_return(75) =", result4)
print("Expected: 75, Actual:", result4)
print("Status:", result4 == 75 and "✅ PASS" or "❌ FAIL")
print("")

print("=== Parameter Return Debug Test Complete ===")

-- Summary
local total_tests = 4
local passed_tests = 0
if result1 == 42 then passed_tests = passed_tests + 1 end
if result2 == 100 then passed_tests = passed_tests + 1 end
if result3 == 20 then passed_tests = passed_tests + 1 end
if result4 == 75 then passed_tests = passed_tests + 1 end

print("Summary: " .. passed_tests .. "/" .. total_tests .. " tests passed")
