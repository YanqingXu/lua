-- Minimal require test to isolate issues
print("=== Minimal require() Test ===")

-- Test 1: Check if require function exists
print("\nTest 1: require function")
if require then
    print("✓ require exists")
    print("  Type:", type(require))
else
    print("✗ require not found")
    return
end

-- Test 2: Check package table
print("\nTest 2: package table")
if package then
    print("✓ package exists")
    print("  Type:", type(package))
else
    print("✗ package not found")
    return
end

-- Test 3: Check package.path
print("\nTest 3: package.path")
if package.path then
    print("✓ package.path exists")
    print("  Value:", package.path)
else
    print("✗ package.path not found")
end

-- Test 4: Check package.loaded
print("\nTest 4: package.loaded")
if package.loaded then
    print("✓ package.loaded exists")
    print("  Type:", type(package.loaded))
else
    print("✗ package.loaded not found")
end

-- Test 5: Try to load string library
print("\nTest 5: require('string')")
local success, result = pcall(require, "string")
if success then
    print("✓ string library loaded")
    print("  Type:", type(result))
else
    print("✗ string library failed to load")
    print("  Error:", result)
end

print("\n=== Minimal Test Complete ===")
