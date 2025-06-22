#pragma once

#include "../test_utils.hpp"
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
        //RUN_TEST_SUITE(CompilerSymbolTableTest);
        //RUN_TEST_SUITE(CompilerLiteralTest);
        //RUN_TEST_SUITE(CompilerVariableTest);
        //RUN_TEST_SUITE(CompilerBinaryExpressionTest);
        //RUN_TEST_SUITE(CompilerConditionalTest);
        //RUN_TEST_SUITE(CompilerMultiReturnTest);
        RUN_TEST_SUITE(CompilerErrorTest);
        
        // TODO: Add other test suites here when available
    }
};

} // namespace Lua::Tests
