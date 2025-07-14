-- Test custom module loading directly
print("=== Custom Module Direct Test ===")

-- Test 1: Load simple custom module directly
print("\nTest 1: require('simple_test_module')")
local simple_module = require("simple_test_module")
print("✓ simple_test_module loaded")
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
end

print("\n=== Custom Module Direct Test Complete ===")
