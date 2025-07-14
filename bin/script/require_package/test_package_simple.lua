-- Simple package library test without pairs iteration
print("=== Simple Package Library Test ===")

-- Test 1: package.preload functionality (already confirmed working)
print("\nTest 1: package.preload functionality")
print("✓ package.preload exists:", package.preload ~= nil)
print("  Type:", type(package.preload))

-- Test 2: package.loaders array (check without iteration)
print("\nTest 2: package.loaders array")
print("✓ package.loaders exists:", package.loaders ~= nil)
print("  Type:", type(package.loaders))

-- Check specific loader indices
if package.loaders[1] then
    print("  Loader 1 exists, type:", type(package.loaders[1]))
else
    print("  Loader 1 not found")
end

if package.loaders[2] then
    print("  Loader 2 exists, type:", type(package.loaders[2]))
else
    print("  Loader 2 not found")
end

-- Test 3: package.searchpath function
print("\nTest 3: package.searchpath function")
print("✓ package.searchpath exists:", package.searchpath ~= nil)
print("  Type:", type(package.searchpath))

-- Test 4: package.loaded basic check
print("\nTest 4: package.loaded basic check")
print("✓ package.loaded exists:", package.loaded ~= nil)
print("  Type:", type(package.loaded))

-- Check if string library is in loaded
if package.loaded["string"] then
    print("✓ string library found in package.loaded")
else
    print("✗ string library not found in package.loaded")
end

-- Test 5: package.path
print("\nTest 5: package.path")
print("✓ package.path exists:", package.path ~= nil)
print("  Type:", type(package.path))
print("  Value:", package.path)

-- Test 6: Multiple require calls (caching test)
print("\nTest 6: Module caching verification")
local math1 = require("math")
local math2 = require("math")
if math1 == math2 then
    print("✓ Module caching works correctly")
else
    print("✗ Module caching failed")
end

print("\n=== Simple Package Library Test Complete ===")
