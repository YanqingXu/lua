-- Focused test for specific package library features
print("=== Package Library Features Test ===")

-- Test 1: package.preload functionality
print("\nTest 1: package.preload functionality")
if package.preload then
    print("✓ package.preload exists")
    print("  Type:", type(package.preload))
    
    -- Test adding a preload function
    package.preload["test_preload"] = function()
        return {
            name = "test_preload",
            loaded_via = "preload"
        }
    end
    print("✓ Added test module to package.preload")
    
    -- Test loading via preload
    local preload_module = require("test_preload")
    if preload_module and preload_module.loaded_via == "preload" then
        print("✓ Package.preload functionality works")
        print("  Module name:", preload_module.name)
        print("  Loaded via:", preload_module.loaded_via)
    else
        print("✗ Package.preload functionality failed")
    end
    
    -- Clean up
    package.loaded["test_preload"] = nil
    package.preload["test_preload"] = nil
    print("✓ Cleaned up test module")
else
    print("✗ package.preload not found")
end

-- Test 2: package.loaders array
print("\nTest 2: package.loaders array")
if package.loaders then
    print("✓ package.loaders exists")
    print("  Type:", type(package.loaders))
    
    -- Check if it's a table and has entries
    local count = 0
    for k, v in pairs(package.loaders) do
        count = count + 1
        print("  Loader", k, "type:", type(v))
    end
    print("  Total loaders:", count)
else
    print("✗ package.loaders not found")
end

-- Test 3: package.searchpath function
print("\nTest 3: package.searchpath function")
if package.searchpath then
    print("✓ package.searchpath exists")
    print("  Type:", type(package.searchpath))
    
    -- Test with non-existent module
    local path, err = package.searchpath("nonexistent_module", package.path)
    if path == nil and err then
        print("✓ package.searchpath correctly returns nil for non-existent module")
        print("  Error message type:", type(err))
    else
        print("✗ package.searchpath behavior unexpected")
        print("  Path:", path)
        print("  Error:", err)
    end
else
    print("✗ package.searchpath not found")
end

-- Test 4: package.loaded manipulation
print("\nTest 4: package.loaded manipulation")
if package.loaded then
    print("✓ package.loaded exists")
    
    -- Count current modules
    local count = 0
    for name, _ in pairs(package.loaded) do
        count = count + 1
    end
    print("  Current loaded modules:", count)
    
    -- Test adding to package.loaded
    package.loaded["test_manual"] = {test = true}
    if package.loaded["test_manual"] then
        print("✓ Successfully added module to package.loaded")
    else
        print("✗ Failed to add module to package.loaded")
    end
    
    -- Test removing from package.loaded
    package.loaded["test_manual"] = nil
    if package.loaded["test_manual"] == nil then
        print("✓ Successfully removed module from package.loaded")
    else
        print("✗ Failed to remove module from package.loaded")
    end
else
    print("✗ package.loaded not found")
end

-- Test 5: Error handling for require
print("\nTest 5: Error handling for require")

-- Test require with invalid module name (this should fail gracefully)
print("Testing require with non-existent module...")
local test_module = require("definitely_nonexistent_module_12345")
print("This line should not be reached if error handling works")

print("\n=== Package Library Features Test Complete ===")
