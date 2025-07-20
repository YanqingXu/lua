-- Debug tostring issue with complex error messages
print("=== tostring Issue Debug ===")

print("Getting error message...")
local error_success, error_msg = pcall(function()
    error("test_error")
end)

print("Error message obtained")
print("Type:", type(error_msg))

print("Testing tostring on error message...")
local str_result = tostring(error_msg)
print("tostring completed")
print("Result type:", type(str_result))

print("Testing string concatenation...")
local concat_result = "Error: " .. tostring(error_msg)
print("Concatenation completed")

print("=== tostring Issue Debug Complete ===")
