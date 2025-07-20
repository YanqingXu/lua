-- Debug test for recursive function issue
print("=== Recursive Function Debug Test ===")

-- Test 1: Simple factorial function
function factorial(n)
    print("factorial(" .. n .. ") called")
    if n <= 1 then
        print("Base case: returning 1")
        return 1
    else
        print("Recursive case: n * factorial(" .. (n-1) .. ")")
        local result = n * factorial(n - 1)
        print("factorial(" .. n .. ") = " .. result)
        return result
    end
end

print("Test 1: Factorial function")
print("Computing factorial(5)...")
local fact5 = factorial(5)
print("factorial(5) =", fact5)
print("Expected: 120, Actual:", fact5)
print("Status:", fact5 == 120 and "✅ PASS" or "❌ FAIL")
print("")

-- Test 2: Simple recursive countdown
function countdown(n)
    print("countdown(" .. n .. ")")
    if n <= 0 then
        print("Countdown finished!")
        return 0
    else
        return countdown(n - 1)
    end
end

print("Test 2: Recursive countdown")
local countdown_result = countdown(3)
print("countdown(3) =", countdown_result)
print("Expected: 0, Actual:", countdown_result)
print("Status:", countdown_result == 0 and "✅ PASS" or "❌ FAIL")
print("")

-- Test 3: Fibonacci sequence
function fibonacci(n)
    print("fibonacci(" .. n .. ")")
    if n <= 1 then
        return n
    else
        return fibonacci(n - 1) + fibonacci(n - 2)
    end
end

print("Test 3: Fibonacci sequence")
local fib5 = fibonacci(5)
print("fibonacci(5) =", fib5)
print("Expected: 5, Actual:", fib5)
print("Status:", fib5 == 5 and "✅ PASS" or "❌ FAIL")
print("")

print("=== Recursive Function Debug Test Complete ===")

-- Summary
local total_tests = 3
local passed_tests = 0
if fact5 == 120 then passed_tests = passed_tests + 1 end
if countdown_result == 0 then passed_tests = passed_tests + 1 end
if fib5 == 5 then passed_tests = passed_tests + 1 end

print("Summary: " .. passed_tests .. "/" .. total_tests .. " tests passed")
