-- Basic require() function and package library test
print("=== Basic require() Function and Package Library Test ===")

-- Test 1: Check if require function exists
print("\nTest 1: Check require function existence")
if require then
    print("✓ require function exists")
    print("  Type:", type(require))
else
    print("✗ require function not found")
    return
end

-- Test 2: Check package table existence and structure
print("\nTest 2: Check package table structure")
if package then
    print("✓ package table exists")
    print("  Type:", type(package))
    
    -- Check package.path
    if package.path then
        print("✓ package.path exists")
        print("  Value:", package.path)
    else
        print("✗ package.path not found")
    end
    
    -- Check package.loaded
    if package.loaded then
        print("✓ package.loaded exists")
        print("  Type:", type(package.loaded))
        print("  Loaded modules:")
        for name, module in pairs(package.loaded) do
            print("    " .. name .. ": " .. type(module))
        end
    else
        print("✗ package.loaded not found")
    end
    
    -- Check package.preload
    if package.preload then
        print("✓ package.preload exists")
        print("  Type:", type(package.preload))
    else
        print("✗ package.preload not found")
    end
    
    -- Check package.loaders (Lua 5.1 compatibility)
    if package.loaders then
        print("✓ package.loaders exists")
        print("  Type:", type(package.loaders))
        print("  Number of loaders:", #package.loaders)
    else
        print("✗ package.loaders not found")
    end
    
    -- Check package.searchpath function
    if package.searchpath then
        print("✓ package.searchpath exists")
        print("  Type:", type(package.searchpath))
    else
        print("✗ package.searchpath not found")
    end
else
    print("✗ package table not found")
    return
end

-- Test 3: Test standard library loading via require
print("\nTest 3: Test standard library loading")
local standard_libs = {"string", "table", "math", "io", "os", "debug"}

for _, lib_name in ipairs(standard_libs) do
    print("Testing require('" .. lib_name .. "')...")
    local success, result = pcall(require, lib_name)
    if success then
        print("✓ " .. lib_name .. " loaded successfully")
        print("  Type:", type(result))
        if type(result) == "table" then
            local count = 0
            for k, v in pairs(result) do
                count = count + 1
            end
            print("  Functions/values:", count)
        end
    else
        print("✗ " .. lib_name .. " failed to load")
        print("  Error:", result)
    end
end

-- Test 4: Test module caching
print("\nTest 4: Test module caching")
print("Loading string library twice...")
local string1 = require("string")
local string2 = require("string")
if string1 == string2 then
    print("✓ Module caching works - same object returned")
else
    print("✗ Module caching failed - different objects returned")
end

-- Test 5: Test package.searchpath functionality
print("\nTest 5: Test package.searchpath functionality")
if package.searchpath then
    -- Test with non-existent module
    local path, err = package.searchpath("nonexistent_module", package.path)
    if path == nil and err then
        print("✓ package.searchpath correctly returns nil for non-existent module")
        print("  Error message:", err)
    else
        print("✗ package.searchpath behavior unexpected for non-existent module")
        print("  Path:", path)
        print("  Error:", err)
    end
else
    print("⚠ package.searchpath not available for testing")
end

print("\n=== Basic require() Test Completed ===")
