-- Basic Package Library Test Script
-- This script tests the fundamental functionality of the require() function
-- and package library system

print("=== Basic Package Library Test ===")

-- Test 1: Check that package table exists
print("Test 1: Package table structure")
assert(type(package) == "table", "package table should exist")
assert(type(package.path) == "string", "package.path should be a string")
assert(type(package.loaded) == "table", "package.loaded should be a table")
assert(type(package.preload) == "table", "package.preload should be a table")
assert(type(package.loaders) == "table", "package.loaders should be a table")
print("✓ Package table structure is correct")

-- Test 2: Check that standard libraries are in package.loaded
print("\nTest 2: Standard libraries in package.loaded")
assert(package.loaded.string ~= nil, "string library should be in package.loaded")
assert(package.loaded.table ~= nil, "table library should be in package.loaded")
assert(package.loaded.math ~= nil, "math library should be in package.loaded")
print("✓ Standard libraries are properly loaded")

-- Test 3: Test package.searchpath function
print("\nTest 3: package.searchpath function")
assert(type(package.searchpath) == "function", "package.searchpath should be a function")

-- Test with a pattern that should not find anything
local result = package.searchpath("nonexistent_module", "./?.lua")
assert(result == nil, "searchpath should return nil for non-existent modules")
print("✓ package.searchpath works correctly")

-- Test 4: Test require function exists and is callable
print("\nTest 4: require function")
assert(type(require) == "function", "require should be a function")

-- Test requiring a standard library (should work from cache)
local string_lib = require("string")
assert(type(string_lib) == "table", "require('string') should return a table")
assert(string_lib == string, "require('string') should return the string library")
print("✓ require function works for standard libraries")

-- Test 5: Test loadfile and dofile functions
print("\nTest 5: loadfile and dofile functions")
assert(type(loadfile) == "function", "loadfile should be a function")
assert(type(dofile) == "function", "dofile should be a function")

-- Test loadfile with non-existent file (should return nil)
local func = loadfile("nonexistent_file.lua")
assert(func == nil, "loadfile should return nil for non-existent files")
print("✓ loadfile and dofile functions exist and handle errors correctly")

-- Test 6: Test package.preload functionality
print("\nTest 6: package.preload functionality")
package.preload["test_preload"] = function()
    return {
        name = "test_preload_module",
        version = "1.0"
    }
end

local preload_module = require("test_preload")
assert(type(preload_module) == "table", "preloaded module should be a table")
assert(preload_module.name == "test_preload_module", "preloaded module should have correct data")
assert(package.loaded["test_preload"] == preload_module, "preloaded module should be cached")
print("✓ package.preload functionality works correctly")

-- Test 7: Test module caching
print("\nTest 7: Module caching")
local preload_module2 = require("test_preload")
assert(preload_module == preload_module2, "require should return the same cached module")
print("✓ Module caching works correctly")

-- Test 8: Test error handling
print("\nTest 8: Error handling")
local ok, err = pcall(require, "definitely_nonexistent_module")
assert(not ok, "require should fail for non-existent modules")
assert(type(err) == "string", "require should return error message")
print("✓ Error handling works correctly")

-- Test 9: Test require with invalid arguments
print("\nTest 9: Invalid arguments")
local ok, err = pcall(require, 123)
assert(not ok, "require should fail with non-string argument")

local ok, err = pcall(require)
assert(not ok, "require should fail with no arguments")
print("✓ Invalid argument handling works correctly")

print("\n=== All Basic Package Library Tests Passed! ===")
print("The package library system is working correctly.")
print("You can now use require() to load modules in your Lua programs.")
