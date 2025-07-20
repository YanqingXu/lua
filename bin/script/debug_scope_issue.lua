-- Debug potential scope/variable conflict issue
print("=== Scope Issue Debug ===")

-- Simulate the comprehensive test environment
local test_count = 0
local passed_count = 0
local failed_count = 0

-- Create many variables like in comprehensive test
local num = 42
local str = "Hello Lua"
local bool = true
local nil_val = nil
local empty_table = {}
local obj = {}
obj.name = "test_object"
obj.value = 100
local literal_table = {x = 10, y = 20, name = "point"}
local array = {1, 2, 3, 4, 5}

-- Create functions like in comprehensive test
function add(a, b)
    return a + b
end

function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

-- Test helper function
local function test_assert(condition, test_name, expected, actual)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        failed_count = failed_count + 1
        local msg = "[Failed] " .. test_name .. " - Failed"
        if expected and actual then
            msg = msg .. " (Expected: " .. tostring(expected) .. ", Actual: " .. tostring(actual) .. ")"
        end
        print(msg)
        return false
    end
end

print("Environment setup completed")

-- Test some basic assertions first
test_assert(true, "basic test", true, true)
test_assert(5 + 3 == 8, "arithmetic test", 8, 5 + 3)

print("Basic tests completed")

-- Now test pcall error catching in this environment
print("Testing pcall in comprehensive environment...")
local error_success, error_msg = pcall(function()
    error("test_error")
end)

print("pcall completed, about to test assertion...")
test_assert(error_success == false, "pcall error catching", false, error_success)
print("First assertion completed")

test_assert(type(error_msg) == "string", "pcall error message type", "string", type(error_msg))
print("Second assertion completed")

print("=== Scope Issue Debug Complete ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
