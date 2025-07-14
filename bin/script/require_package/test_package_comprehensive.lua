-- Comprehensive package library functionality test
print("=== Comprehensive Package Library Test ===")

-- Test 1: Package library API completeness
print("\nTest 1: Package library API completeness")
local expected_globals = {"require", "loadfile", "dofile"}
local expected_package_fields = {"path", "loaded", "preload", "loaders", "searchpath"}

print("Checking global functions:")
for _, func_name in ipairs(expected_globals) do
    if _G[func_name] then
        print("✓ " .. func_name .. " exists (type: " .. type(_G[func_name]) .. ")")
    else
        print("✗ " .. func_name .. " missing")
    end
end

print("Checking package table fields:")
for _, field_name in ipairs(expected_package_fields) do
    if package[field_name] then
        print("✓ package." .. field_name .. " exists (type: " .. type(package[field_name]) .. ")")
    else
        print("✗ package." .. field_name .. " missing")
    end
end

-- Test 2: Package path functionality
print("\nTest 2: Package path functionality")
print("Current package.path:", package.path)

-- Test path modification
local original_path = package.path
package.path = "./?.lua;./test/?.lua;" .. original_path
print("Modified package.path:", package.path)

-- Restore original path
package.path = original_path
print("Restored package.path:", package.path)

-- Test 3: Package.loaded manipulation
print("\nTest 3: Package.loaded manipulation")
print("Current loaded modules count:", 0)
local count = 0
for name, _ in pairs(package.loaded) do
    count = count + 1
end
print("Actual loaded modules count:", count)

-- Test adding to package.loaded
package.loaded["test_module"] = {test = true}
if package.loaded["test_module"] then
    print("✓ Successfully added module to package.loaded")
else
    print("✗ Failed to add module to package.loaded")
end

-- Test removing from package.loaded
package.loaded["test_module"] = nil
if package.loaded["test_module"] == nil then
    print("✓ Successfully removed module from package.loaded")
else
    print("✗ Failed to remove module from package.loaded")
end

-- Test 4: Package.preload functionality
print("\nTest 4: Package.preload functionality")
print("Package.preload type:", type(package.preload))

-- Add a preload function
package.preload["test_preload"] = function()
    return {
        name = "test_preload",
        loaded_via = "preload"
    }
end

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

-- Test 5: Multiple require calls and caching
print("\nTest 5: Multiple require calls and caching")
local start_time = os.clock and os.clock() or 0

-- Load string library multiple times
local string_refs = {}
for i = 1, 5 do
    string_refs[i] = require("string")
end

local end_time = os.clock and os.clock() or 0

-- Check if all references are the same (caching works)
local all_same = true
for i = 2, 5 do
    if string_refs[i] ~= string_refs[1] then
        all_same = false
        break
    end
end

if all_same then
    print("✓ Module caching works correctly")
    if os.clock then
        print("  Time for 5 require calls:", (end_time - start_time) * 1000, "ms")
    end
else
    print("✗ Module caching failed")
end

-- Test 6: Error handling
print("\nTest 6: Error handling")

-- Test require with invalid argument
local success, err = pcall(require, nil)
if not success then
    print("✓ require(nil) correctly throws error")
    print("  Error:", err)
else
    print("✗ require(nil) should have thrown error")
end

-- Test require with number
success, err = pcall(require, 123)
if not success then
    print("✓ require(123) correctly throws error")
    print("  Error:", err)
else
    print("✗ require(123) should have thrown error")
end

print("\n=== Comprehensive Package Library Test Completed ===")
