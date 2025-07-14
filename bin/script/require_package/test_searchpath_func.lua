-- Test package.searchpath function specifically
print("=== Package.searchpath Test ===")

-- Test 1: searchpath with non-existent module
print("\nTest 1: searchpath with non-existent module")
local path, err = package.searchpath("nonexistent_module", package.path)
print("Path:", path)
print("Error:", err)
print("Path is nil:", path == nil)
print("Error type:", type(err))

-- Test 2: searchpath with existing module (if any)
print("\nTest 2: searchpath with simple_test_module")
local path2, err2 = package.searchpath("simple_test_module", package.path)
print("Path:", path2)
print("Error:", err2)

print("\n=== Package.searchpath Test Complete ===")
