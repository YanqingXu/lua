-- Test module loading from modules subdirectory
print("=== Module Path Test ===")

-- Update package.path to include modules subdirectory
local original_path = package.path
package.path = "./bin/script/require_package/modules/?.lua;" .. package.path

print("\nUpdated package.path to include modules directory")
print("New path:", package.path)

-- Test loading simple table module
print("\nTest 1: Loading simple_table_module from modules/")
local mod = require("simple_table_module")
print("Result type:", type(mod))
if type(mod) == "table" then
    print("Module name:", mod.name)
    print("Module version:", mod.version)
end

-- Test loading very simple module
print("\nTest 2: Loading very_simple_module from modules/")
local simple = require("very_simple_module")
print("Result:", simple)
print("Type:", type(simple))

-- Restore original path
package.path = original_path
print("\nRestored original package.path")

print("\n=== Module Path Test Complete ===")
