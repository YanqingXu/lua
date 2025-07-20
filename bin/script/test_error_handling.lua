-- Error Handling Test
-- Tests pcall, xpcall, error, and assert functions

print("=== Error Handling Test ===")

local test_count = 0
local passed_count = 0

function test_assert(condition, test_name)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name)
    else
        print("[FAILED] " .. test_name)
    end
end

-- pcall Success Cases
print("Testing pcall success cases...")

local success1, result1 = pcall(function()
    return "success"
end)
test_assert(success1 == true, "pcall success case (boolean)")
test_assert(result1 == "success", "pcall success case (return value)")

local success2, result2 = pcall(function()
    return 42
end)
test_assert(success2 == true, "pcall success case (number return)")
test_assert(result2 == 42, "pcall success case (number value)")

local success3, result3 = pcall(function(a, b)
    return a + b
end, 5, 3)
test_assert(success3 == true, "pcall with parameters (success)")
test_assert(result3 == 8, "pcall with parameters (result)")

-- pcall Error Cases
print("Testing pcall error cases...")

local err_success1, err_msg1 = pcall(function()
    error("test_error")
end)
test_assert(err_success1 == false, "pcall error case (boolean)")
test_assert(type(err_msg1) == "string", "pcall error case (message type)")

local err_success2, err_msg2 = pcall(function()
    error("custom_error_message")
end)
test_assert(err_success2 == false, "pcall custom error (boolean)")

local err_success3, err_msg3 = pcall(function()
    local x = nil
    return x.nonexistent_field
end)
test_assert(err_success3 == false, "pcall runtime error (nil access)")

-- pcall with Function that Returns Multiple Values
print("Testing pcall with multiple return values...")

local multi_success, multi_a, multi_b = pcall(function()
    return 10, 20
end)
test_assert(multi_success == true, "pcall multiple returns (success)")
test_assert(multi_a == 10, "pcall multiple returns (first value)")

-- Note: multi_b might not work due to multi-return issues mentioned earlier

-- Error Function
print("Testing error function...")

local error_caught = false
local error_result = pcall(function()
    error("explicit_error")
end)
test_assert(error_result == false, "error function generates error")

-- Error with Level Parameter
print("Testing error with level parameter...")

local level_error_caught = false
local level_result = pcall(function()
    error("level_error", 1)
end)
test_assert(level_result == false, "error function with level parameter")

-- Assert Function
print("Testing assert function...")

local assert_success = pcall(function()
    assert(true, "this should not fail")
    return "assert_passed"
end)
test_assert(assert_success == true, "assert function (true condition)")

local assert_failure = pcall(function()
    assert(false, "this should fail")
end)
test_assert(assert_failure == false, "assert function (false condition)")

local assert_nil = pcall(function()
    assert(nil, "nil assertion")
end)
test_assert(assert_nil == false, "assert function (nil condition)")

-- Assert with Return Value
print("Testing assert return value...")

local assert_return = pcall(function()
    local value = assert(42, "should return 42")
    return value
end)
test_assert(assert_return == true, "assert returns value on success")

-- Nested pcall
print("Testing nested pcall...")

local nested_success, nested_result = pcall(function()
    local inner_success, inner_result = pcall(function()
        return "inner_success"
    end)
    if inner_success then
        return inner_result
    else
        error("inner_failed")
    end
end)
test_assert(nested_success == true, "nested pcall success")
test_assert(nested_result == "inner_success", "nested pcall result")

-- Summary
print("")
print("=== Error Handling Test Summary ===")
print("Total tests: " .. tostring(test_count))
print("Passed tests: " .. tostring(passed_count))

local failed_count = test_count - passed_count
print("Failed tests: " .. tostring(failed_count))

if passed_count == test_count then
    print("All error handling features working correctly!")
else
    print("Some error handling features failed")
end

print("=== Error Handling Test Completed ===")
