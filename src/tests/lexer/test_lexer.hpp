#pragma once

#include "../../test_framework/core/test_macros.hpp"
#include "lexer_basic_test.hpp"
#include "lexer_error_test.hpp"

namespace Lua::Tests {

/**
 * @brief Lexer Module Test Suite
 * 
 * This class serves as the MODULE-level test coordinator for all lexer-related functionality.
 * It organizes and runs various test suites that cover different aspects of lexical analysis.
 * 
 * Test Hierarchy within Lexer Module:
 * MODULE (LexerTestSuite) -> SUITE -> GROUP -> INDIVIDUAL
 * 
 * The lexer module includes:
 * - Basic lexical analysis tests (LexerBasicTest)
 * - Error handling tests (LexerErrorTest)
 * - Token recognition tests
 * - Edge case handling tests
 */
class LexerTestSuite {
public:
    /**
     * @brief Run all lexer module tests
     * 
     * Executes all lexer-related test suites in a logical order.
     */
    static void runAllTests() {
        RUN_TEST_SUITE(LexerBasicTest);
        RUN_TEST_SUITE(LexerErrorTest);
        
        // TODO: Add other lexer test suites here when available
        // RUN_TEST_SUITE(LexerTokenTestSuite);
        // RUN_TEST_SUITE(LexerPerformanceTestSuite);
    }
};

} // namespace Lua::Tests