-- Debug error handling specifically
print("=== Error Handling Debug Test ===")

print("Step 1: Basic pcall with success")
local success1, result1 = pcall(function()
    return "success"
end)
print("Success:", success1, "Result:", result1)

print("Step 2: Basic pcall with simple error")
local success2, result2 = pcall(function()
    error("simple error")
end)
print("Success:", success2, "Result:", result2)

print("Step 3: Testing error function directly")
print("About to call error function...")
error("direct error test")
print("This should not be printed")

print("=== Error Handling Debug Complete ===")
