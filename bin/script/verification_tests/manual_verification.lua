#!/usr/bin/env lua
-- Manual Verification Script for Package Library
-- This script provides simple, step-by-step verification that can be run manually
-- to quickly check if the package library implementation is working

print("=== Manual Package Library Verification ===")
print("This script will test the basic functionality of the require() function")
print("and package library system step by step.\n")

local function pause()
    print("Press Enter to continue...")
    io.read()
end

local function check(condition, message)
    if condition then
        print("✓ " .. message)
        return true
    else
        print("✗ " .. message)
        return false
    end
end

print("Step 1: Checking package system structure")
print("----------------------------------------")

local all_good = true

all_good = check(type(package) == "table", "package table exists") and all_good
all_good = check(type(package.path) == "string", "package.path is a string") and all_good
all_good = check(type(package.loaded) == "table", "package.loaded is a table") and all_good
all_good = check(type(package.preload) == "table", "package.preload is a table") and all_good
all_good = check(type(package.loaders) == "table", "package.loaders is a table") and all_good
all_good = check(type(require) == "function", "require function exists") and all_good
all_good = check(type(loadfile) == "function", "loadfile function exists") and all_good
all_good = check(type(dofile) == "function", "dofile function exists") and all_good

if all_good then
    print("\n✅ Package system structure is correct!")
else
    print("\n❌ Package system structure has issues!")
    return
end

pause()

print("\nStep 2: Checking standard library integration")
print("---------------------------------------------")

local std_libs = {"string", "table", "math", "io", "os"}
for _, lib_name in ipairs(std_libs) do
    local lib_in_loaded = package.loaded[lib_name]
    local global_lib = _G[lib_name]
    
    all_good = check(lib_in_loaded ~= nil, lib_name .. " library is in package.loaded") and all_good
    all_good = check(lib_in_loaded == global_lib, lib_name .. " library matches global") and all_good
end

if all_good then
    print("\n✅ Standard library integration is correct!")
else
    print("\n❌ Standard library integration has issues!")
    return
end

pause()

print("\nStep 3: Testing require() with preload")
print("--------------------------------------")

-- Add a simple preload module
package.preload["manual_test"] = function()
    return {
        name = "manual_test",
        value = 42,
        greet = function(name) return "Hello, " .. (name or "World") .. "!" end
    }
end

local test_mod = require("manual_test")

all_good = check(type(test_mod) == "table", "require() returns a table") and all_good
all_good = check(test_mod.name == "manual_test", "module has correct name") and all_good
all_good = check(test_mod.value == 42, "module has correct value") and all_good
all_good = check(type(test_mod.greet) == "function", "module has function") and all_good
all_good = check(test_mod.greet("Manual") == "Hello, Manual!", "module function works") and all_good

-- Test caching
local test_mod2 = require("manual_test")
all_good = check(test_mod == test_mod2, "module is cached (same object returned)") and all_good
all_good = check(package.loaded["manual_test"] == test_mod, "module is in package.loaded") and all_good

if all_good then
    print("\n✅ require() with preload works correctly!")
else
    print("\n❌ require() with preload has issues!")
    return
end

pause()

print("\nStep 4: Testing error handling")
print("------------------------------")

-- Test non-existent module
local ok, err = pcall(require, "definitely_nonexistent_module")
all_good = check(not ok, "require() fails for non-existent module") and all_good
all_good = check(type(err) == "string", "require() returns error message") and all_good
all_good = check(string.find(err, "not found") ~= nil, "error message mentions 'not found'") and all_good

-- Test invalid arguments
local ok2, err2 = pcall(require, 123)
all_good = check(not ok2, "require() fails with invalid argument type") and all_good

local ok3, err3 = pcall(require)
all_good = check(not ok3, "require() fails with no arguments") and all_good

if all_good then
    print("\n✅ Error handling works correctly!")
else
    print("\n❌ Error handling has issues!")
    return
end

pause()

print("\nStep 5: Testing package.searchpath")
print("----------------------------------")

-- Test searchpath with non-existent module
local result = package.searchpath("nonexistent_module", "./?.lua")
all_good = check(result == nil, "searchpath returns nil for non-existent module") and all_good

-- Test searchpath with invalid arguments
local ok4, err4 = pcall(package.searchpath, 123, "./?.lua")
all_good = check(not ok4, "searchpath fails with invalid module name") and all_good

if all_good then
    print("\n✅ package.searchpath works correctly!")
else
    print("\n❌ package.searchpath has issues!")
    return
end

pause()

print("\nStep 6: Testing loadfile and dofile")
print("-----------------------------------")

-- Test loadfile with non-existent file
local func = loadfile("nonexistent_file.lua")
all_good = check(func == nil, "loadfile returns nil for non-existent file") and all_good

-- Test dofile with non-existent file
local ok5, err5 = pcall(dofile, "nonexistent_file.lua")
all_good = check(not ok5, "dofile fails for non-existent file") and all_good
all_good = check(type(err5) == "string", "dofile returns error message") and all_good

if all_good then
    print("\n✅ loadfile and dofile work correctly!")
else
    print("\n❌ loadfile and dofile have issues!")
    return
end

pause()

print("\nStep 7: Testing package.path modification")
print("-----------------------------------------")

local original_path = package.path
package.path = "./?.lua;./test/?.lua"
all_good = check(package.path == "./?.lua;./test/?.lua", "package.path can be modified") and all_good

-- Restore original path
package.path = original_path
all_good = check(package.path == original_path, "package.path can be restored") and all_good

if all_good then
    print("\n✅ package.path modification works correctly!")
else
    print("\n❌ package.path modification has issues!")
    return
end

pause()

print("\nStep 8: Final verification")
print("-------------------------")

-- Test that everything still works after all tests
local string_lib = require("string")
all_good = check(string_lib == string, "Standard library still works") and all_good

local test_mod3 = require("manual_test")
all_good = check(test_mod3 == test_mod, "Custom module still cached") and all_good

-- Count loaded modules
local loaded_count = 0
for _ in pairs(package.loaded) do
    loaded_count = loaded_count + 1
end
all_good = check(loaded_count >= 6, "Multiple modules loaded (" .. loaded_count .. " total)") and all_good

if all_good then
    print("\n✅ Final verification passed!")
else
    print("\n❌ Final verification failed!")
    return
end

print("\n" .. string.rep("=", 50))
print("🎉 MANUAL VERIFICATION COMPLETED SUCCESSFULLY! 🎉")
print(string.rep("=", 50))
print("\nAll basic functionality of the package library is working correctly:")
print("• Package system structure is proper")
print("• Standard libraries are integrated")
print("• require() function works with caching")
print("• Error handling is appropriate")
print("• package.searchpath function works")
print("• loadfile and dofile functions work")
print("• package.path can be modified")
print("• System remains stable after testing")
print("\nThe package library implementation is ready for use!")
print("You can now use require() to load modules in your Lua programs.")

print("\nExample usage:")
print("  local mymodule = require('mymodule')")
print("  local result = mymodule.some_function()")
print("\nFor more advanced testing, run:")
print("  lua test_suite_main.lua")
print("  lua test_file_modules.lua")
print("  lua test_real_world_scenarios.lua")
