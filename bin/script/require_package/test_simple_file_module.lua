-- Test very simple file module loading
print("=== Simple File Module Test ===")

print("\nTesting very_simple_module...")
local simple = require("very_simple_module")
print("Result:", simple)
print("Type:", type(simple))

print("\n=== Simple File Module Test Complete ===")
