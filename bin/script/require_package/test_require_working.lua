-- Test require function directly without pcall
print("=== Working require() Test ===")

-- Test 1: Load string library
print("\nTest 1: require('string')")
local string_lib = require("string")
print("✓ string library loaded")
print("  Type:", type(string_lib))

-- Test 2: Load table library
print("\nTest 2: require('table')")
local table_lib = require("table")
print("✓ table library loaded")
print("  Type:", type(table_lib))

-- Test 3: Load math library
print("\nTest 3: require('math')")
local math_lib = require("math")
print("✓ math library loaded")
print("  Type:", type(math_lib))

-- Test 4: Test module caching
print("\nTest 4: Module caching test")
local string_lib2 = require("string")
if string_lib == string_lib2 then
    print("✓ Module caching works")
else
    print("✗ Module caching failed")
end

-- Test 5: Check package.loaded
print("\nTest 5: package.loaded check")
if package.loaded["string"] then
    print("✓ string found in package.loaded")
else
    print("✗ string not found in package.loaded")
end

print("\n=== Working require() Test Complete ===")
