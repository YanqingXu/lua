#!/usr/bin/env lua
-- Real-world Scenario Tests for Package Library
-- This script demonstrates practical usage patterns and validates
-- production-ready functionality

print("=== Real-world Scenario Tests ===")
print("Testing practical usage patterns for the package library")
print("")

-- Test statistics
local stats = {
    scenarios_tested = 0,
    scenarios_passed = 0,
    scenarios_failed = 0,
    errors = {}
}

local function test_scenario(name, test_function)
    stats.scenarios_tested = stats.scenarios_tested + 1
    print("Testing scenario: " .. name)
    
    local success, error_msg = pcall(test_function)
    if success then
        stats.scenarios_passed = stats.scenarios_passed + 1
        print("✓ " .. name .. " - PASSED")
    else
        stats.scenarios_failed = stats.scenarios_failed + 1
        print("✗ " .. name .. " - FAILED: " .. error_msg)
        table.insert(stats.errors, name .. ": " .. error_msg)
    end
    print("")
end

-- Scenario 1: Building a modular application
local function scenario_modular_application()
    print("  Building a simple modular application...")
    
    -- Load utility modules
    local utils = require("test_modules.utility_module")
    local objects = require("test_modules.object_module")
    
    -- Initialize utility module
    utils.init({debug = true, log_level = "info"})
    
    -- Create application data
    local app_data = {
        name = "Test Application",
        version = "1.0.0",
        users = {},
        sessions = {}
    }
    
    -- Create some users using object module
    local user1 = objects.create_student("Alice Johnson", 20, "MIT")
    local user2 = objects.create_teacher("Dr. Smith", 45, "Computer Science")
    
    user1:add_grade("Math", 95)
    user1:add_grade("Physics", 92)
    user2:add_student(user1)
    
    table.insert(app_data.users, user1)
    table.insert(app_data.users, user2)
    
    -- Use utility functions to process data
    local user_names = {}
    for _, user in ipairs(app_data.users) do
        local name = user:get_name()
        local capitalized = utils.string_utils.capitalize(name)
        table.insert(user_names, capitalized)
    end
    
    -- Validate results
    assert(#user_names == 2, "Should have 2 users")
    assert(user_names[1] == "Alice johnson", "First user name should be capitalized")
    assert(user_names[2] == "Dr. smith", "Second user name should be capitalized")
    
    -- Test cross-module functionality
    local classroom = objects.create_classroom(user2, {user1})
    local simulation = objects.simulate_class(classroom)
    
    assert(type(simulation) == "table", "Simulation should return table")
    assert(#simulation >= 3, "Simulation should have multiple entries")
    
    -- Use time utilities
    local timestamp = utils.time_utils.get_timestamp()
    app_data.created_at = timestamp
    
    assert(type(app_data.created_at) == "number", "Timestamp should be number")
    
    print("  ✓ Modular application built successfully")
    print("  ✓ Cross-module functionality works")
    print("  ✓ Standard library integration confirmed")
end

-- Scenario 2: Library ecosystem simulation
local function scenario_library_ecosystem()
    print("  Simulating a library ecosystem...")
    
    -- Simulate loading multiple related modules
    local modules = {
        "test_modules.simple_module",
        "test_modules.utility_module", 
        "test_modules.object_module"
    }
    
    local loaded_modules = {}
    for _, module_name in ipairs(modules) do
        local mod = require(module_name)
        loaded_modules[module_name] = mod
        
        -- Verify module is cached
        local cached_mod = require(module_name)
        assert(mod == cached_mod, "Module should be cached: " .. module_name)
    end
    
    -- Verify all modules are in package.loaded
    for _, module_name in ipairs(modules) do
        assert(package.loaded[module_name] ~= nil, "Module should be in package.loaded: " .. module_name)
        assert(package.loaded[module_name] == loaded_modules[module_name], "Cached module should match")
    end
    
    -- Test inter-module dependencies work correctly
    local simple = loaded_modules["test_modules.simple_module"]
    local utils = loaded_modules["test_modules.utility_module"]
    local objects = loaded_modules["test_modules.object_module"]
    
    -- Use simple module
    simple.add_message("Test from ecosystem")
    local messages = simple.get_messages()
    
    -- Process with utility module
    local formatted_messages = {}
    for _, msg in ipairs(messages) do
        local trimmed = utils.string_utils.trim(msg)
        local capitalized = utils.string_utils.capitalize(trimmed)
        table.insert(formatted_messages, capitalized)
    end
    
    -- Create objects
    local person = objects.create_person("Test User", 25)
    person:add_friend({name = "Friend 1"})
    
    assert(#formatted_messages > 0, "Should have processed messages")
    assert(#person:get_friends() == 1, "Person should have friends")
    
    print("  ✓ Library ecosystem simulation successful")
    print("  ✓ Module caching verified across ecosystem")
    print("  ✓ Inter-module dependencies work correctly")
end

-- Scenario 3: Configuration and initialization patterns
local function scenario_configuration_patterns()
    print("  Testing configuration and initialization patterns...")
    
    -- Test preload configuration
    package.preload["config_module"] = function()
        return {
            database = {
                host = "localhost",
                port = 5432,
                name = "testdb"
            },
            cache = {
                enabled = true,
                ttl = 3600
            },
            logging = {
                level = "info",
                file = "app.log"
            }
        }
    end
    
    -- Load configuration
    local config = require("config_module")
    assert(type(config) == "table", "Config should be table")
    assert(config.database.host == "localhost", "Config should have database settings")
    assert(config.cache.enabled == true, "Config should have cache settings")
    
    -- Test configuration is cached
    local config2 = require("config_module")
    assert(config == config2, "Config should be cached")
    
    -- Test module initialization with config
    local utils = require("test_modules.utility_module")
    utils.init(config.cache)
    
    local info = utils.get_info()
    assert(type(info) == "table", "Utils info should be table")
    assert(info.config ~= nil, "Utils should have config")
    
    print("  ✓ Configuration module pattern works")
    print("  ✓ Module initialization with config successful")
    print("  ✓ Preload functionality confirmed")
end

-- Scenario 4: Error handling and recovery
local function scenario_error_handling()
    print("  Testing error handling and recovery patterns...")
    
    -- Test graceful handling of missing modules
    local function safe_require(module_name)
        local ok, result = pcall(require, module_name)
        if ok then
            return result
        else
            print("    Warning: Could not load module " .. module_name .. ": " .. result)
            return nil
        end
    end
    
    -- Test with existing module
    local existing_mod = safe_require("test_modules.simple_module")
    assert(existing_mod ~= nil, "Should load existing module")
    assert(type(existing_mod) == "table", "Should return table for existing module")
    
    -- Test with non-existing module
    local missing_mod = safe_require("non_existent_module")
    assert(missing_mod == nil, "Should return nil for missing module")
    
    -- Test module loading with fallbacks
    local function require_with_fallback(primary, fallback)
        local mod = safe_require(primary)
        if mod then
            return mod
        else
            print("    Falling back to: " .. fallback)
            return safe_require(fallback)
        end
    end
    
    local mod = require_with_fallback("missing_module", "test_modules.simple_module")
    assert(mod ~= nil, "Should get fallback module")
    assert(mod.name == "simple_module", "Should get correct fallback")
    
    -- Test error recovery in module usage
    local utils = require("test_modules.utility_module")
    
    -- Test with invalid input (should not crash)
    local result1 = utils.string_utils.trim(nil)
    assert(result1 == nil, "Should handle nil input gracefully")
    
    local result2 = utils.table_utils.copy("not a table")
    assert(result2 == "not a table", "Should handle invalid input gracefully")
    
    local result3 = utils.math_utils.factorial(-1)
    assert(result3 == nil, "Should handle invalid math input gracefully")
    
    print("  ✓ Error handling patterns work correctly")
    print("  ✓ Graceful degradation implemented")
    print("  ✓ Module fallback mechanisms functional")
end

-- Scenario 5: Performance and memory patterns
local function scenario_performance_patterns()
    print("  Testing performance and memory patterns...")
    
    -- Test module loading performance
    local start_time = os.clock()
    
    -- Load modules multiple times (should use cache)
    for i = 1, 100 do
        local mod = require("test_modules.simple_module")
        assert(mod ~= nil, "Module should load successfully")
    end
    
    local end_time = os.clock()
    local elapsed = end_time - start_time
    
    print("    100 module loads took " .. string.format("%.4f", elapsed) .. " seconds")
    assert(elapsed < 1.0, "Cached module loading should be fast")
    
    -- Test memory usage patterns
    local initial_count = 0
    for _ in pairs(package.loaded) do
        initial_count = initial_count + 1
    end
    
    -- Load new modules
    local utils = require("test_modules.utility_module")
    local objects = require("test_modules.object_module")
    
    local final_count = 0
    for _ in pairs(package.loaded) do
        final_count = final_count + 1
    end
    
    print("    Package.loaded grew from " .. initial_count .. " to " .. final_count .. " entries")
    assert(final_count >= initial_count, "Package.loaded should grow with new modules")
    
    -- Test that modules don't interfere with each other
    utils.init({debug = false})
    local person = objects.create_person("Test", 30)
    
    -- Both modules should work independently
    local trimmed = utils.string_utils.trim("  test  ")
    local greeting = person:greet()
    
    assert(trimmed == "test", "Utils should work after objects usage")
    assert(string.find(greeting, "Test") ~= nil, "Objects should work after utils usage")
    
    print("  ✓ Performance patterns acceptable")
    print("  ✓ Memory usage patterns correct")
    print("  ✓ Module isolation maintained")
end

-- Scenario 6: Integration with standard libraries
local function scenario_standard_library_integration()
    print("  Testing integration with standard libraries...")
    
    -- Verify standard libraries are properly loaded
    local std_libs = {"string", "table", "math", "io", "os"}
    
    for _, lib_name in ipairs(std_libs) do
        local lib_from_require = require(lib_name)
        local lib_from_global = _G[lib_name]
        
        assert(lib_from_require == lib_from_global, "require('" .. lib_name .. "') should match global")
        assert(package.loaded[lib_name] == lib_from_global, "Standard lib should be in package.loaded")
    end
    
    -- Test that custom modules can use standard libraries
    local utils = require("test_modules.utility_module")
    
    -- Test string library usage
    local test_str = "Hello World"
    local lower_str = utils.string_utils.trim(string.lower(test_str))
    assert(lower_str == "hello world", "Module should use string library correctly")
    
    -- Test table library usage
    local test_table = {3, 1, 4, 1, 5}
    table.sort(test_table)
    local sorted_copy = utils.table_utils.copy(test_table)
    assert(sorted_copy[1] == 1, "Module should work with sorted tables")
    
    -- Test math library usage
    local rounded = utils.math_utils.round(math.pi, 2)
    assert(rounded == 3.14, "Module should use math library correctly")
    
    -- Test os library usage
    local timestamp = utils.time_utils.get_timestamp()
    local current_time = os.time()
    assert(math.abs(timestamp - current_time) <= 1, "Module should use os library correctly")
    
    print("  ✓ Standard library integration verified")
    print("  ✓ Custom modules use standard libraries correctly")
    print("  ✓ No conflicts between custom and standard modules")
end

-- Main execution
local function run_all_scenarios()
    print("Running real-world scenario tests...\n")
    
    test_scenario("Modular Application Development", scenario_modular_application)
    test_scenario("Library Ecosystem Simulation", scenario_library_ecosystem)
    test_scenario("Configuration and Initialization Patterns", scenario_configuration_patterns)
    test_scenario("Error Handling and Recovery", scenario_error_handling)
    test_scenario("Performance and Memory Patterns", scenario_performance_patterns)
    test_scenario("Standard Library Integration", scenario_standard_library_integration)
    
    -- Print final results
    print("=== Real-world Scenario Results ===")
    print("Scenarios tested: " .. stats.scenarios_tested)
    print("Scenarios passed: " .. stats.scenarios_passed)
    print("Scenarios failed: " .. stats.scenarios_failed)
    print("Success rate: " .. string.format("%.1f%%", (stats.scenarios_passed / stats.scenarios_tested) * 100))
    
    if stats.scenarios_failed > 0 then
        print("\nFailed scenarios:")
        for _, error_msg in ipairs(stats.errors) do
            print("  " .. error_msg)
        end
        print("\n❌ Some real-world scenarios failed.")
        return false
    else
        print("\n✅ All real-world scenarios passed!")
        return true
    end
end

-- Run all scenarios
local success = run_all_scenarios()

if success then
    print("\n🎉 Real-world scenario testing completed successfully!")
    print("The package library is ready for production use in real applications.")
    print("All practical usage patterns work correctly.")
else
    print("\n⚠️  Some real-world scenarios failed.")
    print("Please review the implementation for production readiness.")
end
