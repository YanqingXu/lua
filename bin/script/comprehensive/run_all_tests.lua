#!/usr/bin/env lua
-- Comprehensive Test Runner for Package Library
-- This script runs all test suites and provides a complete verification
-- of the require() function and package library implementation

print("=== Comprehensive Package Library Test Runner ===")
print("Running all test suites to verify package library implementation")
print("Lua version: " .. _VERSION)
print("Test start time: " .. os.date("%Y-%m-%d %H:%M:%S"))
print("")

-- Test configuration
local TEST_CONFIG = {
    run_basic_tests = true,
    run_file_tests = true,
    run_scenario_tests = true,
    verbose = true,
    continue_on_failure = true
}

-- Overall test statistics
local overall_stats = {
    suites_run = 0,
    suites_passed = 0,
    suites_failed = 0,
    total_tests = 0,
    total_passed = 0,
    total_failed = 0,
    start_time = os.clock(),
    errors = {}
}

-- Utility function to run a test suite
local function run_test_suite(name, script_path, description)
    overall_stats.suites_run = overall_stats.suites_run + 1
    
    print("┌" .. string.rep("─", 60) .. "┐")
    print("│ " .. string.format("%-58s", "Running: " .. name) .. " │")
    print("│ " .. string.format("%-58s", description) .. " │")
    print("└" .. string.rep("─", 60) .. "┘")
    
    -- Check if script file exists
    local file = io.open(script_path, "r")
    if not file then
        print("❌ Test suite file not found: " .. script_path)
        overall_stats.suites_failed = overall_stats.suites_failed + 1
        table.insert(overall_stats.errors, name .. ": File not found")
        return false
    end
    file:close()
    
    -- Execute the test suite
    local success, error_msg = pcall(dofile, script_path)
    
    if success then
        print("✅ " .. name .. " completed successfully")
        overall_stats.suites_passed = overall_stats.suites_passed + 1
        return true
    else
        print("❌ " .. name .. " failed: " .. tostring(error_msg))
        overall_stats.suites_failed = overall_stats.suites_failed + 1
        table.insert(overall_stats.errors, name .. ": " .. tostring(error_msg))
        
        if not TEST_CONFIG.continue_on_failure then
            print("Stopping test execution due to failure.")
            return false
        end
        return false
    end
end

-- Function to display system information
local function display_system_info()
    print("=== System Information ===")
    print("Lua Version: " .. _VERSION)
    print("OS: " .. (os.getenv("OS") or "Unknown"))
    print("Current Directory: " .. (io.popen("cd"):read("*l") or "Unknown"))
    
    -- Check package system availability
    print("\nPackage System Check:")
    print("• package table: " .. (type(package) == "table" and "✓" or "✗"))
    print("• require function: " .. (type(require) == "function" and "✓" or "✗"))
    print("• loadfile function: " .. (type(loadfile) == "function" and "✓" or "✗"))
    print("• dofile function: " .. (type(dofile) == "function" and "✓" or "✗"))
    
    if type(package) == "table" then
        print("• package.path: " .. (type(package.path) == "string" and "✓" or "✗"))
        print("• package.loaded: " .. (type(package.loaded) == "table" and "✓" or "✗"))
        print("• package.preload: " .. (type(package.preload) == "table" and "✓" or "✗"))
        print("• package.loaders: " .. (type(package.loaders) == "table" and "✓" or "✗"))
        print("• package.searchpath: " .. (type(package.searchpath) == "function" and "✓" or "✗"))
    end
    
    print("")
end

-- Function to check test file availability
local function check_test_files()
    print("=== Test File Availability Check ===")
    
    local test_files = {
        {"test_suite_main.lua", "Basic functionality tests"},
        {"test_file_modules.lua", "File-based module tests"},
        {"test_real_world_scenarios.lua", "Real-world scenario tests"},
        {"manual_verification.lua", "Manual verification script"},
        {"test_modules/simple_module.lua", "Simple test module"},
        {"test_modules/utility_module.lua", "Utility test module"},
        {"test_modules/object_module.lua", "Object-oriented test module"},
        {"test_modules/circular_a.lua", "Circular dependency test A"},
        {"test_modules/circular_b.lua", "Circular dependency test B"},
        {"test_modules/subdir/nested_module.lua", "Nested module test"}
    }
    
    local all_files_present = true
    
    for _, file_info in ipairs(test_files) do
        local filename, description = file_info[1], file_info[2]
        local file = io.open(filename, "r")
        if file then
            file:close()
            print("✓ " .. filename .. " - " .. description)
        else
            print("✗ " .. filename .. " - " .. description .. " (MISSING)")
            all_files_present = false
        end
    end
    
    if all_files_present then
        print("\n✅ All test files are present")
    else
        print("\n❌ Some test files are missing")
        print("Please ensure all test files are in the correct locations.")
    end
    
    print("")
    return all_files_present
end

-- Main test execution function
local function run_all_tests()
    print("Starting comprehensive package library testing...\n")
    
    -- Display system information
    display_system_info()
    
    -- Check test file availability
    if not check_test_files() then
        print("❌ Cannot proceed with testing due to missing files.")
        return false
    end
    
    local all_suites_passed = true
    
    -- Run basic functionality tests
    if TEST_CONFIG.run_basic_tests then
        local success = run_test_suite(
            "Basic Functionality Tests",
            "test_suite_main.lua",
            "Tests core package system functionality and API"
        )
        all_suites_passed = all_suites_passed and success
        print("")
    end
    
    -- Run file-based module tests
    if TEST_CONFIG.run_file_tests then
        local success = run_test_suite(
            "File-based Module Tests",
            "test_file_modules.lua",
            "Tests require() with actual Lua module files"
        )
        all_suites_passed = all_suites_passed and success
        print("")
    end
    
    -- Run real-world scenario tests
    if TEST_CONFIG.run_scenario_tests then
        local success = run_test_suite(
            "Real-world Scenario Tests",
            "test_real_world_scenarios.lua",
            "Tests practical usage patterns and production scenarios"
        )
        all_suites_passed = all_suites_passed and success
        print("")
    end
    
    return all_suites_passed
end

-- Function to display final results
local function display_final_results(success)
    local end_time = os.clock()
    local total_time = end_time - overall_stats.start_time
    
    print("┌" .. string.rep("═", 70) .. "┐")
    print("│" .. string.format("%70s", " ") .. "│")
    print("│" .. string.format("%-68s", "  COMPREHENSIVE TEST RESULTS") .. "│")
    print("│" .. string.format("%70s", " ") .. "│")
    print("├" .. string.rep("─", 70) .. "┤")
    print("│ Test Execution Summary:" .. string.format("%47s", " ") .. "│")
    print("│   • Test suites run: " .. string.format("%-45s", overall_stats.suites_run) .. "│")
    print("│   • Test suites passed: " .. string.format("%-42s", overall_stats.suites_passed) .. "│")
    print("│   • Test suites failed: " .. string.format("%-42s", overall_stats.suites_failed) .. "│")
    print("│   • Success rate: " .. string.format("%-48s", string.format("%.1f%%", (overall_stats.suites_passed / overall_stats.suites_run) * 100)) .. "│")
    print("│   • Total execution time: " .. string.format("%-38s", string.format("%.2f seconds", total_time)) .. "│")
    print("│" .. string.format("%70s", " ") .. "│")
    
    if success then
        print("│ " .. string.format("%-68s", "🎉 ALL TESTS PASSED! Package library is working correctly.") .. "│")
        print("│ " .. string.format("%-68s", "   The require() function and package system are ready for use.") .. "│")
    else
        print("│ " .. string.format("%-68s", "❌ SOME TESTS FAILED! Please review the implementation.") .. "│")
        if #overall_stats.errors > 0 then
            print("│" .. string.format("%70s", " ") .. "│")
            print("│ Failed test suites:" .. string.format("%47s", " ") .. "│")
            for _, error_msg in ipairs(overall_stats.errors) do
                local truncated = string.sub(error_msg, 1, 64)
                print("│   • " .. string.format("%-61s", truncated) .. "│")
            end
        end
    end
    
    print("│" .. string.format("%70s", " ") .. "│")
    print("└" .. string.rep("═", 70) .. "┘")
    
    print("\nTest completion time: " .. os.date("%Y-%m-%d %H:%M:%S"))
    
    if success then
        print("\n🚀 Package library implementation verified successfully!")
        print("You can now use require() to load modules in your Lua programs.")
        print("\nExample usage:")
        print("  local mymodule = require('mymodule')")
        print("  local utils = require('utils')")
        print("  local config = require('config')")
    else
        print("\n⚠️  Package library implementation needs attention.")
        print("Please review the failed tests and fix any issues before using in production.")
    end
end

-- Execute all tests
local success = run_all_tests()

-- Display final results
display_final_results(success)

-- Return appropriate exit code
if success then
    print("\n✅ Test suite completed successfully!")
else
    print("\n❌ Test suite completed with failures!")
end
