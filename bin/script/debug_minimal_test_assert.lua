-- Minimal test to isolate test_assert issue
print("=== Minimal test_assert Debug ===")

print("Step 1: Testing basic string operations")
local test_str = "test"
print("Basic string:", test_str)
print("tostring(false):", tostring(false))
print("String concatenation:", "Expected: " .. tostring(false))

print("Step 2: Testing pcall results")
local error_success, error_msg = pcall(function()
    error("test_error")
end)
print("pcall completed")
print("error_success:", error_success)

print("Step 3: Testing tostring on pcall results")
print("tostring(error_success):", tostring(error_success))

print("Step 4: Testing string concatenation with pcall results")
local msg = "Expected: " .. tostring(false) .. ", Actual: " .. tostring(error_success)
print("Concatenation result:", msg)

print("=== Minimal test_assert Debug Complete ===")
