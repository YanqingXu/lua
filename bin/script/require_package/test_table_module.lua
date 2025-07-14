-- Test simple table module
print("=== Simple Table Module Test ===")

print("\nTesting simple_table_module...")
local mod = require("simple_table_module")
print("Result type:", type(mod))
if type(mod) == "table" then
    print("Module name:", mod.name)
    print("Module version:", mod.version)
end

print("\n=== Simple Table Module Test Complete ===")
