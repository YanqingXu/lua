-- Debug __call metamethod calculation error
print("=== __call Metamethod Fix Debug ===")

-- Test helper function
local function test_assert(condition, test_name, expected, actual)
    if condition then
        print("[OK] " .. test_name .. " - Passed")
        return true
    else
        print("[Failed] " .. test_name .. " - Failed (Expected: " .. tostring(expected) .. ", Actual: " .. tostring(actual) .. ")")
        return false
    end
end

print("Testing __call metamethod...")

-- __call metamethod
local callable_obj = {}
local callable_meta = {
    __call = function(self, call_x, call_y)
        print("__call function called")
        print("self:", self)
        print("call_x:", call_x)
        print("call_y:", call_y)
        local result = call_x + call_y + 100
        print("Calculated result:", result)
        return result
    end
}
setmetatable(callable_obj, callable_meta)

print("About to call callable_obj(5, 3)...")
local call_result = callable_obj(5, 3)
print("Call completed, result:", call_result)

test_assert(call_result == 108, "__call metamethod", 108, call_result)

print("=== __call Metamethod Fix Debug Complete ===")
