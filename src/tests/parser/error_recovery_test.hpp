#ifndef PARSER_ERROR_RECOVERY_TEST_HPP
#define PARSER_ERROR_RECOVERY_TEST_HPP

#include <iostream>
#include <string>
#include "../../parser/parser.hpp"
#include "../../lexer/lexer.hpp"
#include "../test_utils.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Parser Error Recovery Test Class
 * 
 * This class tests the error recovery mechanisms in the parser,
 * including synchronization, balanced delimiter skipping, and
 * recovery reporting functionality.
 * 
 * Test Coverage:
 * - Basic synchronization after syntax errors
 * - Balanced delimiter skipping (parentheses, brackets, braces)
 * - Error recovery in different parsing contexts
 * - Recovery statistics and reporting
 * - Edge cases and malformed input handling
 */
class ParserErrorRecoveryTest {
public:
    /**
     * @brief Run all error recovery tests
     * 
     * Executes all test groups for parser error recovery functionality.
     */
    static void runAllTests();
    
private:
    // Test groups
    static void testBasicSynchronization();
    static void testBalancedDelimiterSkipping();
    static void testErrorRecoveryInExpressions();
    static void testErrorRecoveryInStatements();
    static void testRecoveryReporting();
    static void testEdgeCases();
    
    // Individual test methods
    static void testSynchronizeAfterMissingSemicolon();
    static void testSynchronizeAfterInvalidToken();
    static void testSynchronizeWithNestedStructures();
    
    static void testSkipBalancedParentheses();
    static void testSkipBalancedBrackets();
    static void testSkipBalancedBraces();
    static void testSkipNestedDelimiters();
    static void testSkipUnbalancedDelimiters();
    
    static void testRecoveryInBinaryExpressions();
    static void testRecoveryInFunctionCalls();
    static void testRecoveryInTableConstructors();
    
    static void testRecoveryInIfStatements();
    static void testRecoveryInWhileStatements();
    static void testRecoveryInFunctionDefinitions();
    
    static void testRecoveryStatistics();
    static void testRecoveryMessages();
    
    static void testEmptyInput();
    static void testOnlyErrorTokens();
    static void testVeryLongErrorSequence();
    
    // Helper methods
    static void printTestResult(const std::string& testName, bool passed);
    static bool parseAndCheckRecovery(const std::string& source, bool expectRecovery = true);
    static bool containsExpectedError(const std::string& source, const std::string& expectedError);
};

} // namespace Tests
} // namespace Lua

#endif // PARSER_ERROR_RECOVERY_TEST_HPP