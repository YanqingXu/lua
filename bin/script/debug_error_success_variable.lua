-- Debug error_success variable issue
print("=== error_success Variable Debug ===")

print("Step 1: pcall error catching")
local error_success, error_msg = pcall(function()
    error("test_error")
end)
print("pcall completed")

print("Step 2: Checking error_success variable")
print("error_success value:", error_success)
print("error_success type:", type(error_success))

print("Step 3: Testing equality comparison")
local comparison_result = (error_success == false)
print("error_success == false:", comparison_result)

print("Step 4: Testing direct boolean check")
if error_success then
    print("error_success is truthy")
else
    print("error_success is falsy")
end

print("Step 5: Testing with explicit false")
local explicit_false = false
print("explicit_false:", explicit_false)
print("explicit_false == false:", explicit_false == false)

print("Step 6: Testing comparison types")
print("type(error_success):", type(error_success))
print("type(false):", type(false))

print("=== error_success Variable Debug Complete ===")
