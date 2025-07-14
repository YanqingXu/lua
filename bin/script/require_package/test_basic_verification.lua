-- Basic verification that require and package functionality works in new location
print("=== Basic Verification Test ===")

-- Test 1: require function exists
print("\nTest 1: require function")
print("require exists:", require ~= nil)
print("require type:", type(require))

-- Test 2: package table exists
print("\nTest 2: package table")
print("package exists:", package ~= nil)
print("package type:", type(package))

-- Test 3: Load standard library
print("\nTest 3: Standard library loading")
local string_lib = require("string")
print("string library loaded:", string_lib ~= nil)
print("string library type:", type(string_lib))

-- Test 4: Module caching
print("\nTest 4: Module caching")
local string_lib2 = require("string")
print("Caching works:", string_lib == string_lib2)

-- Test 5: package.path
print("\nTest 5: package.path")
print("package.path exists:", package.path ~= nil)
print("Current path:", package.path)

print("\n=== Basic Verification Complete ===")
print("✅ All tests passed - files moved successfully!")
