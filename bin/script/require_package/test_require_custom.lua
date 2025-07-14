-- Test custom module loading with require()
print("=== Custom Module Loading Test ===")

-- Test 1: Load simple custom module
print("\nTest 1: Load simple custom module")
local success, simple_module = pcall(require, "simple_test_module")

if success then
    print("✓ simple_test_module loaded successfully")
    print("  Type:", type(simple_module))
    
    if type(simple_module) == "table" then
        print("  Module name:", simple_module.name or "unknown")
        print("  Module version:", simple_module.version or "unknown")
        
        -- Test module functions
        if simple_module.hello then
            print("  Testing hello():", simple_module.hello())
        end
        
        if simple_module.add then
            print("  Testing add(5, 3):", simple_module.add(5, 3))
        end
        
        if simple_module.get_info then
            local info = simple_module.get_info()
            print("  Module info:")
            for k, v in pairs(info) do
                print("    " .. k .. ":", v)
            end
        end
    end
else
    print("✗ simple_test_module failed to load")
    print("  Error:", simple_module)
end

-- Test 2: Test module caching for custom modules
print("\nTest 2: Test custom module caching")
if success then
    local simple_module2 = require("simple_test_module")
    if simple_module == simple_module2 then
        print("✓ Custom module caching works")
    else
        print("✗ Custom module caching failed")
    end
end

-- Test 3: Check if module is in package.loaded
print("\nTest 3: Check package.loaded for custom module")
if package.loaded["simple_test_module"] then
    print("✓ Custom module found in package.loaded")
    print("  Type:", type(package.loaded["simple_test_module"]))
else
    print("✗ Custom module not found in package.loaded")
end

-- Test 4: Test loading non-existent module
print("\nTest 4: Test loading non-existent module")
local success_bad, error_msg = pcall(require, "non_existent_module_12345")
if not success_bad then
    print("✓ Non-existent module correctly failed to load")
    print("  Error:", error_msg)
else
    print("✗ Non-existent module unexpectedly loaded")
end

print("\n=== Custom Module Loading Test Completed ===")
