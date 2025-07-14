#!/usr/bin/env lua
-- File-based Module Testing Suite
-- This script tests require() functionality with actual Lua files

print("=== File-based Module Testing Suite ===")
print("Testing require() with actual Lua module files")
print("")

-- Test configuration
local TEST_CONFIG = {
    verbose = true,
    cleanup_on_exit = false
}

-- Test statistics
local stats = {
    total_tests = 0,
    passed_tests = 0,
    failed_tests = 0,
    errors = {}
}

-- Utility functions
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

-- Test 1: Simple module loading
local function test_simple_module()
    local simple_mod = require("test_modules.simple_module")
    
    test_assert(type(simple_mod) == "table", "simple_module returns table", "Simple Module Type")
    test_assert(simple_mod.name == "simple_module", "simple_module has correct name", "Simple Module Name")
    test_assert(simple_mod.version == "1.0.0", "simple_module has correct version", "Simple Module Version")
    test_assert(type(simple_mod.increment) == "function", "simple_module has increment function", "Simple Module Function")
    
    -- Test module functionality
    local count1 = simple_mod.increment()
    test_assert(count1 == 1, "increment returns 1 on first call", "Simple Module Increment 1")
    
    local count2 = simple_mod.increment()
    test_assert(count2 == 2, "increment returns 2 on second call", "Simple Module Increment 2")
    
    test_assert(simple_mod.get_counter() == 2, "get_counter returns correct value", "Simple Module Counter")
    
    -- Test string functions
    local greeting = simple_mod.greet("Lua")
    test_assert(greeting == "Hello, Lua!", "greet function works correctly", "Simple Module Greet")
    
    -- Test calculation function
    local result = simple_mod.calculate(10, 5, "add")
    test_assert(result == 15, "calculate function works correctly", "Simple Module Calculate")
    
    -- Test message functions
    local msg_count = simple_mod.add_message("Test message")
    test_assert(msg_count == 1, "add_message returns correct count", "Simple Module Message Count")
    
    local messages = simple_mod.get_messages()
    test_assert(#messages == 1, "get_messages returns correct array", "Simple Module Messages")
    test_assert(messages[1] == "Test message", "message content is correct", "Simple Module Message Content")
end

-- Test 2: Utility module with standard library usage
local function test_utility_module()
    local util_mod = require("test_modules.utility_module")
    
    test_assert(type(util_mod) == "table", "utility_module returns table", "Utility Module Type")
    test_assert(util_mod._NAME == "utility_module", "utility_module has correct name", "Utility Module Name")
    
    -- Test string utilities
    test_assert(type(util_mod.string_utils) == "table", "string_utils exists", "Utility String Utils")
    
    local trimmed = util_mod.string_utils.trim("  hello world  ")
    test_assert(trimmed == "hello world", "trim function works", "Utility Trim")
    
    local split_result = util_mod.string_utils.split("a,b,c", ",")
    test_assert(#split_result == 3, "split returns correct count", "Utility Split Count")
    test_assert(split_result[1] == "a", "split first element correct", "Utility Split Element")
    
    local capitalized = util_mod.string_utils.capitalize("hello")
    test_assert(capitalized == "Hello", "capitalize function works", "Utility Capitalize")
    
    -- Test table utilities
    test_assert(type(util_mod.table_utils) == "table", "table_utils exists", "Utility Table Utils")
    
    local original = {a = 1, b = 2}
    local copied = util_mod.table_utils.copy(original)
    test_assert(copied.a == 1 and copied.b == 2, "copy function works", "Utility Table Copy")
    test_assert(copied ~= original, "copy creates new table", "Utility Table Copy New")
    
    local keys = util_mod.table_utils.keys({x = 1, y = 2, z = 3})
    test_assert(#keys == 3, "keys function returns correct count", "Utility Table Keys")
    
    -- Test math utilities
    test_assert(type(util_mod.math_utils) == "table", "math_utils exists", "Utility Math Utils")
    
    local clamped = util_mod.math_utils.clamp(15, 1, 10)
    test_assert(clamped == 10, "clamp function works", "Utility Math Clamp")
    
    local rounded = util_mod.math_utils.round(3.14159, 2)
    test_assert(rounded == 3.14, "round function works", "Utility Math Round")
    
    local factorial = util_mod.math_utils.factorial(5)
    test_assert(factorial == 120, "factorial function works", "Utility Math Factorial")
    
    -- Test validation utilities
    test_assert(type(util_mod.validation) == "table", "validation exists", "Utility Validation")
    
    local is_email = util_mod.validation.is_email("test@example.com")
    test_assert(is_email == true, "email validation works for valid email", "Utility Email Valid")
    
    local is_not_email = util_mod.validation.is_email("invalid-email")
    test_assert(is_not_email == false, "email validation works for invalid email", "Utility Email Invalid")
    
    -- Test time utilities (using os library)
    test_assert(type(util_mod.time_utils) == "table", "time_utils exists", "Utility Time Utils")
    
    local timestamp = util_mod.time_utils.get_timestamp()
    test_assert(type(timestamp) == "number", "get_timestamp returns number", "Utility Timestamp")
    
    local date_str = util_mod.time_utils.get_current_date()
    test_assert(type(date_str) == "string", "get_current_date returns string", "Utility Date")
    test_assert(string.len(date_str) == 10, "date string has correct length", "Utility Date Length")
end

-- Test 3: Object-oriented module
local function test_object_module()
    local obj_mod = require("test_modules.object_module")
    
    test_assert(type(obj_mod) == "table", "object_module returns table", "Object Module Type")
    test_assert(type(obj_mod.Person) == "table", "Person class exists", "Object Person Class")
    test_assert(type(obj_mod.Student) == "table", "Student class exists", "Object Student Class")
    test_assert(type(obj_mod.Teacher) == "table", "Teacher class exists", "Object Teacher Class")
    
    -- Test Person class
    local person = obj_mod.create_person("John", 30)
    test_assert(type(person) == "table", "create_person returns table", "Object Person Create")
    test_assert(person:get_name() == "John", "person has correct name", "Object Person Name")
    test_assert(person:get_age() == 30, "person has correct age", "Object Person Age")
    
    local greeting = person:greet()
    test_assert(type(greeting) == "string", "person greet returns string", "Object Person Greet")
    test_assert(string.find(greeting, "John") ~= nil, "greeting contains name", "Object Person Greet Content")
    
    -- Test Student class (inheritance)
    local student = obj_mod.create_student("Alice", 20, "University")
    test_assert(type(student) == "table", "create_student returns table", "Object Student Create")
    test_assert(student:get_name() == "Alice", "student inherits name functionality", "Object Student Name")
    test_assert(student:get_school() == "University", "student has school", "Object Student School")
    
    local grade_added = student:add_grade("Math", 95)
    test_assert(grade_added == true, "add_grade returns true", "Object Student Add Grade")
    
    local math_grade = student:get_grade("Math")
    test_assert(math_grade == 95, "get_grade returns correct value", "Object Student Get Grade")
    
    student:add_grade("Science", 88)
    local average = student:calculate_average()
    test_assert(average == 91.5, "calculate_average works correctly", "Object Student Average")
    
    -- Test Teacher class
    local teacher = obj_mod.create_teacher("Ms. Smith", 35, "Mathematics")
    test_assert(type(teacher) == "table", "create_teacher returns table", "Object Teacher Create")
    test_assert(teacher:get_subject() == "Mathematics", "teacher has subject", "Object Teacher Subject")
    
    local student_added = teacher:add_student(student)
    test_assert(student_added == true, "add_student returns true", "Object Teacher Add Student")
    test_assert(teacher:get_student_count() == 1, "student count is correct", "Object Teacher Count")
    
    -- Test classroom functionality
    local classroom = obj_mod.create_classroom(teacher, {student})
    test_assert(type(classroom) == "table", "create_classroom returns table", "Object Classroom Create")
    test_assert(classroom.teacher == teacher, "classroom has correct teacher", "Object Classroom Teacher")
    test_assert(#classroom.students == 1, "classroom has correct student count", "Object Classroom Students")
end

-- Test 4: Module caching with file modules
local function test_file_module_caching()
    -- Clear any existing cache for this test
    package.loaded["test_modules.simple_module"] = nil
    
    local mod1 = require("test_modules.simple_module")
    local mod2 = require("test_modules.simple_module")
    
    test_assert(mod1 == mod2, "same module returns same object", "File Module Caching Same Object")
    test_assert(package.loaded["test_modules.simple_module"] == mod1, "module cached in package.loaded", "File Module Caching Loaded")
    
    -- Test that module state is preserved
    mod1.increment()
    mod1.increment()
    local count1 = mod1.get_counter()
    
    local count2 = mod2.get_counter()
    test_assert(count1 == count2, "module state preserved across requires", "File Module Caching State")
    test_assert(count1 == 2, "counter has expected value", "File Module Caching Value")
end

-- Test 5: Package.path modification for subdirectory
local function test_subdirectory_module()
    local original_path = package.path
    
    -- Add test_modules subdirectory to path
    package.path = package.path .. ";test_modules/subdir/?.lua"
    
    local nested_mod = require("nested_module")
    test_assert(type(nested_mod) == "table", "nested module loads successfully", "Subdirectory Module Load")
    test_assert(nested_mod.name == "nested_module", "nested module has correct name", "Subdirectory Module Name")
    test_assert(nested_mod.location == "test_modules/subdir", "nested module has correct location", "Subdirectory Module Location")
    
    -- Test nested module functionality
    local count = nested_mod.add_item("test item")
    test_assert(count == 1, "nested module add_item works", "Subdirectory Module Add Item")
    
    local items = nested_mod.get_items()
    test_assert(#items == 1, "nested module get_items works", "Subdirectory Module Get Items")
    test_assert(items[1] == "test item", "nested module item content correct", "Subdirectory Module Item Content")
    
    -- Test that nested module can use standard libraries
    local formatted = nested_mod.format_items()
    test_assert(type(formatted) == "table", "nested module format_items works", "Subdirectory Module Format")
    test_assert(#formatted == 1, "formatted items has correct count", "Subdirectory Module Format Count")
    
    local timestamp = nested_mod.get_timestamp()
    test_assert(type(timestamp) == "string", "nested module get_timestamp works", "Subdirectory Module Timestamp")
    
    -- Restore original path
    package.path = original_path
end

-- Test 6: Circular dependency detection
local function test_circular_dependency()
    -- This test should fail due to circular dependency
    local ok, err = pcall(require, "test_modules.circular_a")
    
    test_assert(not ok, "circular dependency detected", "Circular Dependency Detection")
    test_assert(type(err) == "string", "circular dependency returns error message", "Circular Dependency Error")
    test_assert(string.find(string.lower(err), "circular") ~= nil, "error message mentions circular dependency", "Circular Dependency Message")
end

-- Test 7: loadfile and dofile functionality
local function test_loadfile_dofile()
    -- Test loadfile
    local func = loadfile("test_modules/simple_module.lua")
    test_assert(type(func) == "function", "loadfile returns function", "Loadfile Function")
    
    local result = func()
    test_assert(type(result) == "table", "loadfile result is table", "Loadfile Result")
    test_assert(result.name == "simple_module", "loadfile result has correct content", "Loadfile Content")
    
    -- Test dofile
    local dofile_result = dofile("test_modules/simple_module.lua")
    test_assert(type(dofile_result) == "table", "dofile returns table", "Dofile Result")
    test_assert(dofile_result.name == "simple_module", "dofile result has correct content", "Dofile Content")
    
    -- Test loadfile with non-existent file
    local nil_func = loadfile("nonexistent_file.lua")
    test_assert(nil_func == nil, "loadfile returns nil for non-existent file", "Loadfile Non-existent")
    
    -- Test dofile with non-existent file
    local ok, err = pcall(dofile, "nonexistent_file.lua")
    test_assert(not ok, "dofile fails for non-existent file", "Dofile Non-existent")
    test_assert(type(err) == "string", "dofile returns error message", "Dofile Error")
end

-- Main test execution
local function run_all_tests()
    print("Starting file-based module tests...\n")
    
    run_test_section("Simple Module Loading", test_simple_module)
    run_test_section("Utility Module with Standard Libraries", test_utility_module)
    run_test_section("Object-Oriented Module", test_object_module)
    run_test_section("File Module Caching", test_file_module_caching)
    run_test_section("Subdirectory Module Loading", test_subdirectory_module)
    run_test_section("Circular Dependency Detection", test_circular_dependency)
    run_test_section("Loadfile and Dofile Functionality", test_loadfile_dofile)
    
    -- Print final results
    print("\n=== File-based Test Results ===")
    print("Total tests: " .. stats.total_tests)
    print("Passed: " .. stats.passed_tests)
    print("Failed: " .. stats.failed_tests)
    print("Success rate: " .. string.format("%.1f%%", (stats.passed_tests / stats.total_tests) * 100))
    
    if stats.failed_tests > 0 then
        print("\nFailed tests:")
        for _, error_msg in ipairs(stats.errors) do
            print("  " .. error_msg)
        end
        print("\n❌ Some file-based tests failed.")
        return false
    else
        print("\n✅ All file-based tests passed!")
        return true
    end
end

-- Run the test suite
local success = run_all_tests()

if success then
    print("\n🎉 File-based module testing completed successfully!")
    print("The require() function works correctly with actual Lua module files.")
else
    print("\n⚠️  Some file-based tests failed. Please check the implementation.")
end
