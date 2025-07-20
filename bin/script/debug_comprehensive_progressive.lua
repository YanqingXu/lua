-- Progressive comprehensive test to find the exact failure point
print("=== Progressive Comprehensive Test ===")

-- Test statistics
local test_count = 0
local passed_count = 0
local failed_count = 0

-- Test helper function
local function test_assert(condition, test_name, expected, actual)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name)
        return true
    else
        failed_count = failed_count + 1
        local msg = "[Failed] " .. test_name
        if expected and actual then
            msg = msg .. " (Expected: " .. tostring(expected) .. ", Actual: " .. tostring(actual) .. ")"
        end
        print(msg)
        return false
    end
end

print("Step 1: Basic operations")
test_assert(5 + 3 == 8, "Addition", 8, 5 + 3)
print("Step 1 completed")

print("Step 2: Simple function")
function simple_func(x)
    return x * 2
end
test_assert(simple_func(5) == 10, "Simple function", 10, simple_func(5))
print("Step 2 completed")

print("Step 3: Table operations")
local t = {1, 2, 3}
test_assert(#t == 3, "Table length", 3, #t)
print("Step 3 completed")

print("Step 4: Simple closure (no upvalues)")
function create_simple()
    return function()
        return 42
    end
end
local simple_closure = create_simple()
test_assert(simple_closure() == 42, "Simple closure", 42, simple_closure())
print("Step 4 completed")

print("Step 5: Closure with upvalues")
function create_counter(start)
    local count = start
    return function()
        count = count + 1
        return count
    end
end
local counter = create_counter(10)
local count1 = counter()
test_assert(count1 == 11, "Counter closure first call", 11, count1)
print("Step 5 completed")

print("Step 6: Multiple closure calls")
local count2 = counter()
test_assert(count2 == 12, "Counter closure second call", 12, count2)
print("Step 6 completed")

print("Step 7: Recursive function")
function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end
local fact5 = factorial(5)
test_assert(fact5 == 120, "Factorial function", 120, fact5)
print("Step 7 completed")

print("Step 8: Complex expression")
local complex_result = (5 + 3) * 2 - 1
test_assert(complex_result == 15, "Complex expression", 15, complex_result)
print("Step 8 completed")

print("Step 9: String operations")
local str_result = "Hello" .. " " .. "World"
test_assert(str_result == "Hello World", "String concatenation", "Hello World", str_result)
print("Step 9 completed")

print("Step 10: Control flow")
local control_result = 0
for i = 1, 5 do
    if i > 3 then
        break
    end
    control_result = control_result + i
end
test_assert(control_result == 6, "Control flow with break", 6, control_result)
print("Step 10 completed")

-- Final results
print("=== Progressive Test Results ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("Success Rate: " .. (passed_count / test_count * 100) .. "%")
print("Progressive test completed successfully!")
print("=== End Progressive Test ===")
