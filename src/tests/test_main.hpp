#ifndef LUA_TESTS_MAIN_NEW_HPP
#define LUA_TESTS_MAIN_NEW_HPP

// Test framework core components
#include "../test_framework/core/test_macros.hpp"
#include "../test_framework/core/test_utils.hpp"

// Include all test module header files
#include "lexer/test_lexer.hpp"
#include "vm/test_vm.hpp"
#include "parser/test_parser.hpp"
#include "compiler/test_compiler.hpp"
#include "gc/test_gc.hpp"
#include "lib/test_lib.hpp"
#include "localization/localization_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Main test class - using the new test framework
 * 
 * This is the main entry point for the refactored test framework.
 * Provides clearer modular test execution and better error handling.
 */
class MainTestSuite {
public:
    /**
     * @brief Run all test modules
     * 
     * This is the main entry point for the entire test suite.
     * Uses the new test framework to run tests for all major modules.
     * 
     * Test hierarchy:
     * MAIN (runAllTests) -> MODULE (module test suites) -> SUITE -> GROUP -> INDIVIDUAL
     */
    static void runAllTests() {
        using namespace Lua::TestFramework;
        
        // Configure test framework
        TestUtils::setColorEnabled(true);
        TestUtils::setMemoryCheckEnabled(true);
        
        RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
        RUN_TEST_MODULE("Parser Module", ParserTestSuite);
        //RUN_TEST_MODULE("Compiler Module", CompilerTestSuite);
        //RUN_TEST_MODULE("VM Module", VMTestSuite);
        //RUN_TEST_MODULE("GC Module", GCTestSuite);
        //RUN_TEST_MODULE("Library Module", LibTestSuite);
        //RUN_TEST_MODULE("Localization Module", LocalizationTestSuite);
        //RUN_TEST_MODULE("Plugin Module", PluginTestSuite);
    }
    
    /**
     * @brief Run tests for a specific module
     * @param moduleName Module name
     */
    static void runModuleTests(const std::string& moduleName) {
        using namespace Lua::TestFramework;
        
        TestUtils::setColorEnabled(true);
        TestUtils::setMemoryCheckEnabled(true);
        
        if (moduleName == "lexer") {
            RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
        } else if (moduleName == "parser") {
            RUN_TEST_MODULE("Parser Module", ParserTestSuite);
        } else if (moduleName == "compiler") {
            RUN_TEST_MODULE("Compiler Module", CompilerTestSuite);
        } else if (moduleName == "vm") {
            RUN_TEST_MODULE("VM Module", VMTestSuite);
        } else if (moduleName == "gc") {
            RUN_TEST_MODULE("GC Module", GCTestSuite);
        } else if (moduleName == "lib") {
            RUN_TEST_MODULE("Library Module", LibTestSuite);
        //} else if (moduleName == "localization") {
        //    RUN_TEST_MODULE("Localization Module", LocalizationTestSuite);
        //} else if (moduleName == "plugin") {
        //    RUN_TEST_MODULE("Plugin Module", PluginTestSuite);
        } else {
            TestUtils::printError("Unknown module: " + moduleName);
            TestUtils::printInfo("Available modules: lexer, parser, compiler, vm, gc, lib, localization, plugin");
        }
    }
    
    /**
     * @brief Run quick tests (core functionality only)
     */
    static void runQuickTests() {
        using namespace Lua::TestFramework;
        
        TestUtils::setColorEnabled(true);
        TestUtils::setMemoryCheckEnabled(false); // Quick tests skip memory checks
        
        TestUtils::printInfo("Running quick tests (core functionality only)...");
        
        RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
        RUN_TEST_MODULE("Parser Module", ParserTestSuite);
        RUN_TEST_MODULE("VM Module", VMTestSuite);
    }
};

/**
 * @brief Convenience function: run specific module tests
 * @param moduleName Module name
 */
inline void runModuleTests(const std::string& moduleName) {
    MainTestSuite::runModuleTests(moduleName);
}

/**
 * @brief Convenience function: run quick tests
 */
inline void runQuickTests() {
    RUN_MAIN_TEST("Lua Interpreter Quick Test Suite", MainTestSuite::runQuickTests);
}

} // namespace Tests
} // namespace Lua

#endif // LUA_TESTS_MAIN_NEW_HPP