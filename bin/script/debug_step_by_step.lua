-- Step by step debug test
print("=== Step by Step Debug Test ===")

-- Test statistics
local test_count = 0
local passed_count = 0
local failed_count = 0

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

print("Step 1: Basic functionality")
test_assert(5 + 3 == 8, "Addition", 8, 5 + 3)
print("Step 1 completed")

print("Step 2: String operations")
test_assert(string.len("hello") == 5, "String length", 5, string.len("hello"))
print("Step 2 completed")

print("Step 3: Table operations")
local t = {}
t.key = "value"
test_assert(t.key == "value", "Table assignment", "value", t.key)
print("Step 3 completed")

print("Step 4: Function calls")
function test_func()
    return 42
end
test_assert(test_func() == 42, "Function call", 42, test_func())
print("Step 4 completed")

print("Step 5: Metatable basic")
local obj = {}
local mt = {}
setmetatable(obj, mt)
test_assert(getmetatable(obj) == mt, "Metatable basic", "same", "same")
print("Step 5 completed")

print("Step 6: Math functions")
test_assert(math.abs(-5) == 5, "Math abs", 5, math.abs(-5))
print("Step 6 completed")

print("Step 7: Control flow")
local result = ""
if true then
    result = "true_branch"
else
    result = "false_branch"
end
test_assert(result == "true_branch", "If statement", "true_branch", result)
print("Step 7 completed")

print("Step 8: For loop")
local sum = 0
for i = 1, 3 do
    sum = sum + i
end
test_assert(sum == 6, "For loop", 6, sum)
print("Step 8 completed")

print("Step 9: Closure test")
function create_func()
    local x = 10
    return function()
        return x
    end
end
local closure = create_func()
test_assert(closure() == 10, "Closure", 10, closure())
print("Step 9 completed")

print("Step 10: Error handling")
local success, err = pcall(function()
    return "success"
end)
test_assert(success == true, "pcall success", true, success)
print("Step 10 completed")

-- Summary
print("")
print("=== Summary ===")
print("Total tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("All steps completed successfully!")
print("=== Debug test completed ===")
