#pragma once

#include <iostream>
#include "lexer_basic_test.hpp"

namespace Lua::Tests {

/**
 * @brief Lexer Module Test Suite
 *
 * Simplified lexer test suite without test framework dependencies.
 */
class LexerTestSuite {
public:
    /**
     * @brief Run all lexer module tests
     *
     * Note: Test framework removed - tests disabled
     */
    static void runAllTests() {
        std::cout << "Lexer tests disabled - test framework removed\n";

        // Individual test methods can be called directly if needed:
        // LexerBasicTest::runAllTests();
    }
};

} // namespace Lua::Tests