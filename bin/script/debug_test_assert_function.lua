-- Debug test_assert function issue
print("=== test_assert Function Debug ===")

-- Test statistics
local test_count = 0
local passed_count = 0
local failed_count = 0

-- Test helper function (exact copy)
local function test_assert(condition, test_name, expected, actual)
    print("=== test_assert called ===")
    print("condition:", condition)
    print("test_name:", test_name)
    print("expected:", expected)
    print("actual:", actual)
    
    test_count = test_count + 1
    print("test_count incremented to:", test_count)
    
    if condition then
        passed_count = passed_count + 1
        print("Condition is true, incrementing passed_count to:", passed_count)
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        failed_count = failed_count + 1
        print("Condition is false, incrementing failed_count to:", failed_count)
        local msg = "[Failed] " .. test_name .. " - Failed"
        print("Base message created:", msg)
        
        if expected and actual then
            print("Adding expected/actual to message...")
            print("About to call tostring on expected:", expected)
            local expected_str = tostring(expected)
            print("Expected string:", expected_str)
            
            print("About to call tostring on actual:", actual)
            local actual_str = tostring(actual)
            print("Actual string:", actual_str)
            
            msg = msg .. " (Expected: " .. expected_str .. ", Actual: " .. actual_str .. ")"
            print("Final message:", msg)
        end
        
        print("About to print final message...")
        print(msg)
        print("Message printed successfully")
        return false
    end
end

print("Step 1: Testing with simple values")
test_assert(true, "simple true test", "expected", "expected")

print("Step 2: Testing with pcall results")
local error_success, error_msg = pcall(function()
    error("test_error")
end)

print("Step 3: About to call test_assert with pcall results")
test_assert(error_success == false, "pcall error catching", false, error_success)

print("=== test_assert Function Debug Complete ===")
