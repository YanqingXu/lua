﻿#include "../../../lib/package/package_lib.hpp"
#include "../../../lib/package/file_utils.hpp"
#include "../../../lib/core/lib_manager.hpp"
#include "../../../vm/state.hpp"
#include "../../../vm/table.hpp"
#include "../../../vm/function.hpp"
#include "../../../common/defines.hpp"
#include <iostream>
#include <fstream>
#include <cassert>
#include <filesystem>

namespace Lua {

/**
 * @brief Test suite for PackageLib functionality
 *
 * Tests cover:
 * - Basic require() functionality
 * - Module caching behavior
 * - package.path handling
 * - package.loaded table
 * - package.preload functionality
 * - Error cases and edge conditions
 * - Circular dependency detection
 * - File system integration
 */
class PackageLibTest {
public:
    static void runAllTests() {
        std::cout << "=== PackageLib Test Suite ===" << std::endl;
        
        testPackageLibInitialization();
        testPackageTableStructure();
        testFileUtilities();
        testBasicRequire();
        testModuleCaching();
        testPackagePath();
        testPreloadFunctionality();
        testErrorCases();
        testCircularDependency();
        testSearchPath();
        testLoadfileDofile();
        
        std::cout << "=== All PackageLib Tests Passed! ===" << std::endl;
    }

private:
    // ===================================================================
    // Test Helper Functions
    // ===================================================================

    static void createTestModule(const std::string& filename, const std::string& content) {
        std::filesystem::create_directories(std::filesystem::path(filename).parent_path());
        std::ofstream file(filename);
        if (file.is_open()) {
            file << content;
            file.close();
        }
    }

    static void cleanupTestFiles() {
        // Clean up test files
        std::filesystem::remove("test_module.lua");
        std::filesystem::remove("test_module2.lua");
        std::filesystem::remove("circular_a.lua");
        std::filesystem::remove("circular_b.lua");
        std::filesystem::remove_all("test_dir");
    }

    // ===================================================================
    // Test Cases
    // ===================================================================

    static void testPackageLibInitialization() {
        std::cout << "Testing PackageLib initialization..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        // Check that package table exists
        Value packageTable = state.getGlobal("package");
        assert(packageTable.isTable());
        
        // Check that require function exists
        Value requireFunc = state.getGlobal("require");
        assert(requireFunc.isFunction());
        
        // Check that loadfile function exists
        Value loadfileFunc = state.getGlobal("loadfile");
        assert(loadfileFunc.isFunction());
        
        // Check that dofile function exists
        Value dofileFunc = state.getGlobal("dofile");
        assert(dofileFunc.isFunction());
        
        std::cout << "✓ PackageLib initialization test passed" << std::endl;
    }

    static void testPackageTableStructure() {
        std::cout << "Testing package table structure..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        Value packageTable = state.getGlobal("package");
        auto table = packageTable.asTable();
        
        // Check package.path
        Value path = table->get(Value("path"));
        assert(path.isString());
        assert(!path.toString().empty());
        
        // Check package.loaded
        Value loaded = table->get(Value("loaded"));
        assert(loaded.isTable());
        
        // Check package.preload
        Value preload = table->get(Value("preload"));
        assert(preload.isTable());
        
        // Check package.loaders
        Value loaders = table->get(Value("loaders"));
        assert(loaders.isTable());
        
        // Check that standard libraries are in package.loaded
        auto loadedTable = loaded.asTable();
        Value stringLib = loadedTable->get(Value("string"));
        assert(!stringLib.isNil());
        
        std::cout << "✓ Package table structure test passed" << std::endl;
    }

    static void testFileUtilities() {
        std::cout << "Testing file utilities..." << std::endl;
        
        // Test file existence checking
        assert(!FileUtils::fileExists("nonexistent_file.lua"));
        
        // Create a test file
        createTestModule("test_file.lua", "-- test file\nreturn 42");
        assert(FileUtils::fileExists("test_file.lua"));
        
        // Test file reading
        std::string content = FileUtils::readFile("test_file.lua");
        assert(content.find("return 42") != std::string::npos);
        
        // Test path manipulation
        assert(FileUtils::joinPath("dir", "file.lua") == "dir/file.lua" || 
               FileUtils::joinPath("dir", "file.lua") == "dir\\file.lua");
        
        assert(FileUtils::getFilename("dir/file.lua") == "file.lua");
        assert(FileUtils::getDirectory("dir/file.lua") == "dir");
        assert(FileUtils::getExtension("file.lua") == ".lua");
        
        // Test module name to path conversion
        assert(FileUtils::moduleNameToPath("foo.bar") == "foo/bar");
        
        // Clean up
        std::filesystem::remove("test_file.lua");
        
        std::cout << "✓ File utilities test passed" << std::endl;
    }

    static void testBasicRequire() {
        std::cout << "Testing basic require functionality..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        // Create a simple test module
        createTestModule("test_module.lua", 
            "local M = {}\n"
            "M.value = 42\n"
            "M.greet = function(name) return 'Hello, ' .. name end\n"
            "return M\n");
        
        // Test require
        bool success = state.doString("local mod = require('test_module')");
        assert(success);
        
        // Test that module was loaded correctly
        success = state.doString("assert(mod.value == 42)");
        assert(success);
        
        success = state.doString("assert(mod.greet('World') == 'Hello, World')");
        assert(success);
        
        cleanupTestFiles();
        std::cout << "✓ Basic require test passed" << std::endl;
    }

    static void testModuleCaching() {
        std::cout << "Testing module caching..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        // Create a module that tracks how many times it's loaded
        createTestModule("cache_test.lua",
            "if not _G.load_count then _G.load_count = 0 end\n"
            "_G.load_count = _G.load_count + 1\n"
            "return { count = _G.load_count }\n");
        
        // Require the module multiple times
        bool success = state.doString(
            "local mod1 = require('cache_test')\n"
            "local mod2 = require('cache_test')\n"
            "assert(mod1.count == 1)\n"  // Should only load once
            "assert(mod1 == mod2)\n"     // Should be the same object
        );
        assert(success);
        
        cleanupTestFiles();
        std::cout << "✓ Module caching test passed" << std::endl;
    }

    static void testPackagePath() {
        std::cout << "Testing package.path functionality..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        // Create module in subdirectory
        createTestModule("test_dir/submodule.lua", "return { name = 'submodule' }");
        
        // Modify package.path to include test directory
        bool success = state.doString("package.path = package.path .. ';test_dir/?.lua'");
        assert(success);
        
        // Test requiring module from subdirectory
        success = state.doString(
            "local sub = require('submodule')\n"
            "assert(sub.name == 'submodule')\n"
        );
        assert(success);
        
        cleanupTestFiles();
        std::cout << "✓ Package.path test passed" << std::endl;
    }

    static void testPreloadFunctionality() {
        std::cout << "Testing package.preload functionality..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        // Add a preloaded module
        bool success = state.doString(
            "package.preload['preloaded'] = function()\n"
            "  return { type = 'preloaded' }\n"
            "end\n"
        );
        assert(success);
        
        // Test requiring preloaded module
        success = state.doString(
            "local pre = require('preloaded')\n"
            "assert(pre.type == 'preloaded')\n"
        );
        assert(success);
        
        std::cout << "✓ Package.preload test passed" << std::endl;
    }

    static void testErrorCases() {
        std::cout << "Testing error cases..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        // Test requiring non-existent module
        bool success = state.doString(
            "local ok, err = pcall(require, 'nonexistent_module')\n"
            "assert(not ok)\n"
            "assert(type(err) == 'string')\n"
        );
        assert(success);
        
        // Test require with invalid argument
        success = state.doString(
            "local ok, err = pcall(require, 123)\n"
            "assert(not ok)\n"
        );
        assert(success);
        
        std::cout << "✓ Error cases test passed" << std::endl;
    }

    static void testCircularDependency() {
        std::cout << "Testing circular dependency detection..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        // Create circular dependency
        createTestModule("circular_a.lua", "return require('circular_b')");
        createTestModule("circular_b.lua", "return require('circular_a')");
        
        // Test that circular dependency is detected
        bool success = state.doString(
            "local ok, err = pcall(require, 'circular_a')\n"
            "assert(not ok)\n"
            "assert(string.find(err, 'circular'))\n"
        );
        assert(success);
        
        cleanupTestFiles();
        std::cout << "✓ Circular dependency test passed" << std::endl;
    }

    static void testSearchPath() {
        std::cout << "Testing package.searchpath..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        // Create test file
        createTestModule("search_test.lua", "return {}");
        
        // Test searchpath function
        bool success = state.doString(
            "local path = package.searchpath('search_test', './?.lua')\n"
            "assert(type(path) == 'string')\n"
            "assert(string.find(path, 'search_test.lua'))\n"
        );
        assert(success);
        
        // Test searchpath with non-existent module
        success = state.doString(
            "local path = package.searchpath('nonexistent', './?.lua')\n"
            "assert(path == nil)\n"
        );
        assert(success);
        
        cleanupTestFiles();
        std::cout << "✓ Package.searchpath test passed" << std::endl;
    }

    static void testLoadfileDofile() {
        std::cout << "Testing loadfile and dofile..." << std::endl;
        
        State state;
        StandardLibrary::initializeAll(&state);
        
        // Create test file
        createTestModule("loadfile_test.lua", 
            "_G.loadfile_executed = true\n"
            "return 'loadfile_result'\n");
        
        // Test loadfile
        bool success = state.doString(
            "local func = loadfile('loadfile_test.lua')\n"
            "assert(type(func) == 'function')\n"
            "assert(_G.loadfile_executed == nil)\n"  // Should not execute yet
            "local result = func()\n"
            "assert(result == 'loadfile_result')\n"
            "assert(_G.loadfile_executed == true)\n"
        );
        assert(success);
        
        // Reset global
        state.setGlobal("loadfile_executed", Value());
        
        // Test dofile
        success = state.doString(
            "local result = dofile('loadfile_test.lua')\n"
            "assert(result == 'loadfile_result')\n"
            "assert(_G.loadfile_executed == true)\n"
        );
        assert(success);
        
        cleanupTestFiles();
        std::cout << "✓ Loadfile and dofile test passed" << std::endl;
    }
};

} // namespace Lua

// Test runner
int main() {
    try {
        Lua::PackageLibTest::runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
