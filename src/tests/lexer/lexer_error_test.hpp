#ifndef LEXER_ERROR_TEST_HPP
#define LEXER_ERROR_TEST_HPP

#include <iostream>
#include <string>
#include "../../lexer/lexer.hpp"
#include "../test_utils.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Lexer Error Handling Test Suite
 * 
 * This class serves as the SUITE-level test coordinator for lexer error handling functionality.
 * It organizes and runs various test groups that cover error detection and handling mechanisms.
 * 
 * Test Hierarchy:
 * MODULE (LexerTestSuite) -> SUITE (LexerErrorTest) -> GROUP -> INDIVIDUAL
 * 
 * Test Coverage:
 * - Invalid character handling
 * - Unterminated string literals
 * - Malformed number literals
 * - Invalid escape sequences
 * - Unicode handling errors
 * - Edge cases and boundary conditions
 */
class LexerErrorTest {
public:
    /**
     * @brief Run all lexer error handling tests
     * 
     * Executes all test groups for lexer error handling functionality.
     */
    static void runAllTests();
    
private:
    // Test groups
    static void testInvalidCharacters();
    static void testStringErrors();
    static void testNumberErrors();
    static void testEscapeSequenceErrors();
    static void testEdgeCases();
    
    // Individual test methods
    static void testInvalidSymbols();
    static void testInvalidUnicodeCharacters();
    static void testControlCharacters();
    
    static void testUnterminatedString();
    static void testUnterminatedMultilineString();
    static void testInvalidStringEscapes();
    
    static void testMalformedNumbers();
    static void testInvalidHexNumbers();
    static void testNumberOverflow();
    
    static void testInvalidEscapeSequences();
    static void testIncompleteEscapeSequences();
    
    static void testEmptyInput();
    static void testOnlyWhitespace();
    static void testVeryLongTokens();
    static void testMixedValidInvalidTokens();
    
    // Helper methods
    static void printTestResult(const std::string& testName, bool passed);
    static bool lexAndCheckError(const std::string& source, TokenType expectedErrorType = TokenType::Error);
    static bool containsErrorToken(const std::string& source);
    static int countErrorTokens(const std::string& source);
};

} // namespace Tests
} // namespace Lua

#endif // LEXER_ERROR_TEST_HPP