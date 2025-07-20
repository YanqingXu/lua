-- Simple pcall test without complex string operations
print("=== Simple pcall Test ===")

print("Testing pcall error catching...")
local error_success, error_msg = pcall(function()
    error("test_error")
end)

print("pcall completed")
print("Success:", error_success)
print("Message type:", type(error_msg))

-- Test without string concatenation
if error_success == false then
    print("Error catching: PASS")
else
    print("Error catching: FAIL")
end

if type(error_msg) == "string" then
    print("Error message type: PASS")
else
    print("Error message type: FAIL")
end

print("=== Simple pcall Test Complete ===")
