-- Test the __call metamethod specifically
print("=== __call Metamethod Test ===")

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

print("Testing __call metamethod...")

-- __call metamethod
local callable_obj = {}
print("Created callable object")

local callable_meta = {
    __call = function(self, call_x, call_y)
        print("__call function called with:", call_x, call_y)
        return call_x + call_y + 100
    end
}
print("Created callable metatable")

setmetatable(callable_obj, callable_meta)
print("Set metatable")

print("About to call callable_obj(5, 3)...")
local call_result = callable_obj(5, 3)
print("Call completed, result:", call_result)

test_assert(call_result == 108, "__call metamethod", 108, call_result)

print("=== __call Metamethod Test Results ===")
print("Total Tests: " .. test_count)
print("Passed: " .. passed_count)
print("Failed: " .. failed_count)
print("=== __call Metamethod Test Complete ===")
