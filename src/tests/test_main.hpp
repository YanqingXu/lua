#ifndef LUA_TESTS_MAIN_NEW_HPP
#define LUA_TESTS_MAIN_NEW_HPP

// Test framework core components
#include "../test_framework/core/test_macros.hpp"
#include "../test_framework/core/test_utils.hpp"

// Include all test module header files
//#include "lexer/test_lexer.hpp"
//#include "vm/test_vm.hpp"
//#include "parser/test_parser.hpp"
//#include "compiler/test_compiler.hpp"
//#include "gc/test_gc.hpp"
#include "lib/test_lib.hpp"
//#include "localization/localization_test.hpp"

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
        
        //RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
        //RUN_TEST_MODULE("Parser Module", ParserTestSuite);
        //RUN_TEST_MODULE("Compiler Module", CompilerTestSuite);
        //RUN_TEST_MODULE("VM Module", VMTestSuite);
        //RUN_TEST_MODULE("GC Module", GCTestSuite);
        RUN_TEST_MODULE("Library Module", LibTestModule);
        //RUN_TEST_MODULE("Localization Module", LocalizationTestSuite);
        //RUN_TEST_MODULE("Plugin Module", PluginTestSuite);
    }
};
} // namespace Tests
} // namespace Lua

#endif // LUA_TESTS_MAIN_NEW_HPP