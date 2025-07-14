-- Debug require function argument passing
print("=== Require Arguments Test ===")

-- Test 1: Check argument types
print("\nTest 1: Argument type check")
local module_name = "string"
print("module_name =", module_name)
print("type(module_name) =", type(module_name))

-- Test 2: Try require with explicit string variable
print("\nTest 2: require with variable")
print("About to call require with variable...")
local result = require(module_name)
print("require call completed")
print("result type:", type(result))

print("\n=== Require Arguments Test Complete ===")
