#pragma once

#include <iostream>
#include <string>
#include "../../lexer/lexer.hpp"
// Note: test_lexer_error.hpp is included in test_lexer.hpp
#include "../../test_framework/core/test_macros.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Lexer Basic Test Suite
 * 
 * This class serves as the SUITE-level test coordinator for basic lexer functionality.
 * It organizes and runs various test groups that cover fundamental lexical analysis.
 * 
 * Test Hierarchy:
 * MODULE (LexerTestSuite) -> SUITE (LexerBasicTest) -> GROUP -> INDIVIDUAL
 * 
 * The basic lexer tests include:
 * - Basic lexical analysis tests
 * - Token recognition tests
 * - Fundamental parsing operations
 */
class LexerBasicTest {
public:
    /**
     * @brief Run all lexer module tests
     * 
     * Executes all lexer-related test suites in a logical order.
     */
    static void runAllTests();
    
private:
    static void testBasicLexing();
    static void testTokenRecognition();
    static void testLexer(const std::string& source);
};

/**
 * @brief Legacy Lexer Test Class
 * 
 * Maintains backward compatibility with existing test structure.
 * @deprecated Use LexerBasicTest instead
 */
class LexerTest {
public:
    static void runAllTests();
    
private:
    static void testLexer(const std::string& source);
};

} // namespace Tests
} // namespace Lua