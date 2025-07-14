#!/usr/bin/env lua
-- Main Test Suite for Package Library Implementation
-- This script runs all tests and provides a comprehensive verification
-- of the require() function and package library system

print("=== Lua Package Library Test Suite ===")
print("Testing require() function and package library implementation")
print("Lua version compatibility: 5.1")
print("")

-- Test configuration
local TEST_CONFIG = {
    verbose = true,
    stop_on_error = false,
    cleanup_files = true
}

-- Test statistics
local stats = {
    total_tests = 0,
    passed_tests = 0,
    failed_tests = 0,
    errors = {}
}

-- Utility functions for testing
local function test_assert(condition, message, test_name)
    stats.total_tests = stats.total_tests + 1
    if condition then
        stats.passed_tests = stats.passed_tests + 1
        if TEST_CONFIG.verbose then
            print("✓ " .. (test_name or "Test") .. ": " .. (message or "PASSED"))
        end
        return true
    else
        stats.failed_tests = stats.failed_tests + 1
        local error_msg = "✗ " .. (test_name or "Test") .. ": " .. (message or "FAILED")
        print(error_msg)
        table.insert(stats.errors, error_msg)
        if TEST_CONFIG.stop_on_error then
            error("Test failed: " .. message)
        end
        return false
    end
end

local function run_test_section(name, test_function)
    print("\n--- " .. name .. " ---")
    local success, error_msg = pcall(test_function)
    if not success then
        print("✗ Test section failed: " .. error_msg)
        table.insert(stats.errors, "Section '" .. name .. "' failed: " .. error_msg)
        stats.failed_tests = stats.failed_tests + 1
    end
end

-- Test 1: Basic Package System Structure
local function test_basic_structure()
    test_assert(type(package) == "table", "package table exists", "Package Table")
    test_assert(type(package.path) == "string", "package.path is string", "Package Path")
    test_assert(type(package.loaded) == "table", "package.loaded is table", "Package Loaded")
    test_assert(type(package.preload) == "table", "package.preload is table", "Package Preload")
    test_assert(type(package.loaders) == "table", "package.loaders is table", "Package Loaders")
    test_assert(type(package.searchpath) == "function", "package.searchpath is function", "Package Searchpath")
    
    test_assert(type(require) == "function", "require function exists", "Require Function")
    test_assert(type(loadfile) == "function", "loadfile function exists", "Loadfile Function")
    test_assert(type(dofile) == "function", "dofile function exists", "Dofile Function")
end

-- Test 2: Standard Libraries in package.loaded
local function test_standard_libraries()
    local std_libs = {"string", "table", "math", "io", "os"}
    
    for _, lib_name in ipairs(std_libs) do
        local lib_in_loaded = package.loaded[lib_name]
        local global_lib = _G[lib_name]
        
        test_assert(lib_in_loaded ~= nil, 
                   lib_name .. " library in package.loaded", 
                   "Standard Library Loaded")
        test_assert(lib_in_loaded == global_lib, 
                   lib_name .. " library matches global", 
                   "Standard Library Consistency")
    end
end

-- Test 3: Package.searchpath functionality
local function test_searchpath()
    -- Test with non-existent module
    local result = package.searchpath("nonexistent_module_xyz", "./?.lua")
    test_assert(result == nil, "searchpath returns nil for non-existent module", "Searchpath Non-existent")
    
    -- Test with invalid arguments
    local ok, err = pcall(package.searchpath, 123, "./?.lua")
    test_assert(not ok, "searchpath fails with invalid module name", "Searchpath Invalid Args")
    
    -- Test with empty path
    local result2 = package.searchpath("test", "")
    test_assert(result2 == nil, "searchpath returns nil for empty path", "Searchpath Empty Path")
end

-- Test 4: Basic require functionality with preload
local function test_basic_require()
    -- Test requiring standard library
    local string_lib = require("string")
    test_assert(type(string_lib) == "table", "require('string') returns table", "Require Standard Lib")
    test_assert(string_lib == string, "require('string') returns string library", "Require Standard Lib Match")
    
    -- Test preload functionality
    package.preload["test_preload_module"] = function()
        return {
            name = "test_preload",
            value = 42,
            greet = function(name) return "Hello, " .. (name or "World") end
        }
    end
    
    local preload_mod = require("test_preload_module")
    test_assert(type(preload_mod) == "table", "preload module returns table", "Preload Module Type")
    test_assert(preload_mod.name == "test_preload", "preload module has correct data", "Preload Module Data")
    test_assert(preload_mod.value == 42, "preload module has correct value", "Preload Module Value")
    test_assert(type(preload_mod.greet) == "function", "preload module has function", "Preload Module Function")
    test_assert(preload_mod.greet("Test") == "Hello, Test", "preload module function works", "Preload Module Function Call")
    
    -- Test caching
    local preload_mod2 = require("test_preload_module")
    test_assert(preload_mod == preload_mod2, "preload module is cached", "Preload Module Caching")
    test_assert(package.loaded["test_preload_module"] == preload_mod, "preload module in package.loaded", "Preload Module Loaded")
end

-- Test 5: Error handling
local function test_error_handling()
    -- Test non-existent module
    local ok, err = pcall(require, "definitely_nonexistent_module_xyz")
    test_assert(not ok, "require fails for non-existent module", "Require Non-existent")
    test_assert(type(err) == "string", "require returns error message", "Require Error Message")
    test_assert(string.find(err, "not found") ~= nil, "error message mentions 'not found'", "Require Error Content")
    
    -- Test invalid arguments
    local ok2, err2 = pcall(require, 123)
    test_assert(not ok2, "require fails with non-string argument", "Require Invalid Type")
    
    local ok3, err3 = pcall(require)
    test_assert(not ok3, "require fails with no arguments", "Require No Args")
    
    -- Test loadfile with non-existent file
    local func = loadfile("nonexistent_file_xyz.lua")
    test_assert(func == nil, "loadfile returns nil for non-existent file", "Loadfile Non-existent")
    
    -- Test dofile with non-existent file
    local ok4, err4 = pcall(dofile, "nonexistent_file_xyz.lua")
    test_assert(not ok4, "dofile fails for non-existent file", "Dofile Non-existent")
end

-- Test 6: Package.path modification
local function test_package_path()
    local original_path = package.path
    
    -- Test that package.path can be modified
    package.path = "./?.lua;./test/?.lua"
    test_assert(package.path == "./?.lua;./test/?.lua", "package.path can be modified", "Package Path Modification")
    
    -- Restore original path
    package.path = original_path
    test_assert(package.path == original_path, "package.path restored", "Package Path Restore")
end

-- Test 7: Multiple require calls (caching test)
local function test_caching_behavior()
    -- Create a preload module that tracks load count
    local load_count = 0
    package.preload["cache_test_module"] = function()
        load_count = load_count + 1
        return {
            load_count = load_count,
            timestamp = os.time()
        }
    end
    
    local mod1 = require("cache_test_module")
    local mod2 = require("cache_test_module")
    local mod3 = require("cache_test_module")
    
    test_assert(mod1.load_count == 1, "module loaded only once", "Caching Load Count")
    test_assert(mod1 == mod2, "second require returns same object", "Caching Same Object 1")
    test_assert(mod2 == mod3, "third require returns same object", "Caching Same Object 2")
    test_assert(package.loaded["cache_test_module"] == mod1, "cached module in package.loaded", "Caching Package Loaded")
end

-- Main test execution
local function run_all_tests()
    print("Starting comprehensive package library tests...\n")
    
    run_test_section("Basic Package System Structure", test_basic_structure)
    run_test_section("Standard Libraries Integration", test_standard_libraries)
    run_test_section("Package.searchpath Functionality", test_searchpath)
    run_test_section("Basic Require Functionality", test_basic_require)
    run_test_section("Error Handling", test_error_handling)
    run_test_section("Package.path Modification", test_package_path)
    run_test_section("Module Caching Behavior", test_caching_behavior)
    
    -- Print final results
    print("\n=== Test Results ===")
    print("Total tests: " .. stats.total_tests)
    print("Passed: " .. stats.passed_tests)
    print("Failed: " .. stats.failed_tests)
    print("Success rate: " .. string.format("%.1f%%", (stats.passed_tests / stats.total_tests) * 100))
    
    if stats.failed_tests > 0 then
        print("\nFailed tests:")
        for _, error_msg in ipairs(stats.errors) do
            print("  " .. error_msg)
        end
        print("\n❌ Some tests failed. Please check the implementation.")
        return false
    else
        print("\n✅ All tests passed! Package library implementation is working correctly.")
        return true
    end
end

-- Run the test suite
local success = run_all_tests()

-- Exit with appropriate code
if success then
    print("\n🎉 Package library implementation verified successfully!")
    print("The require() function and package system are ready for production use.")
else
    print("\n⚠️  Package library implementation needs attention.")
    print("Please review the failed tests and fix any issues.")
end
