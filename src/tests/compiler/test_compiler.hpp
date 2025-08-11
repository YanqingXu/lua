#pragma once

#include <iostream>
#include "compiler_symbol_table_test.hpp"
//#include "compiler_literal_test.hpp"
#include "compiler_variable_test.hpp"
//#include "compiler_binary_expression_test.hpp"
//#include "compiler_conditional_test.hpp"
#include "compiler_multi_return_test.hpp"
#include "compiler_error_test.hpp"

namespace Lua::Tests {

/**
 * @brief Compiler Test Suite
 * 
 * Coordinates all compiler related tests
 * This is a MODULE level test coordinator that organizes
 * all compiler-related test suites using the unified test framework.
 */
class CompilerTestSuite {
public:
    /**
     * @brief Run all compiler tests
     * 
     * Execute all test suites in this module using the standardized
     * RUN_TEST_SUITE macro for consistent formatting and error handling.
     */
    static void runAllTests() {
        // Note: Test framework removed - convert to simple function calls if needed
        //CompilerSymbolTableTest::runAllTests();
        //CompilerLiteralTest::runAllTests();
        //CompilerVariableTest::runAllTests();
        //CompilerBinaryExpressionTest::runAllTests();
        //CompilerConditionalTest::runAllTests();
        //CompilerMultiReturnTest::runAllTests();
        //CompilerErrorTest::runAllTests();

        std::cout << "Compiler tests disabled - test framework removed\n";
    }
};

} // namespace Lua::Tests
