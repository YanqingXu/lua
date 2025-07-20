-- Debug type() function issue
print("=== type() Function Debug ===")

print("Testing type() on simple values...")
print("type(42):", type(42))
print("type('hello'):", type("hello"))
print("type(true):", type(true))

print("Getting error message...")
local error_success, error_msg = pcall(function()
    error("test_error")
end)

print("Error message obtained")
print("About to call type() on error message...")

-- This is where the problem occurs
local msg_type = type(error_msg)
print("type() completed, result:", msg_type)

print("=== type() Function Debug Complete ===")
