#ifndef ENHANCED_PARSER_ERROR_TEST_HPP
#define ENHANCED_PARSER_ERROR_TEST_HPP

#include <string>

namespace Lua {
    /**
     * @brief Enhanced Parser Error Test Suite
     * 
     * This class provides comprehensive tests for the enhanced Parser::error() methods,
     * including detailed error reporting, error type classification, location tracking,
     * and integration with the ErrorReporter system.
     */
    class EnhancedParserErrorTest {
    public:
        /**
         * @brief Run all enhanced parser error tests
         * 
         * Executes all enhanced parser error-related test cases in a logical order.
         * Tests cover:
         * - Basic error reporting functionality
         * - Detailed error messages with types and descriptions
         * - Error type classification
         * - Error location tracking
         * - Multiple error handling
         * - Error recovery mechanisms
         * - Integration with consume() method
         * - Lexical error handling
         */
        static void runAllTests();
        
    private:
        /**
         * @brief Print test header
         * @param testName Name of the test being executed
         */
        static void printTestHeader(const std::string& testName);
        
        /**
         * @brief Print test footer
         * @param testName Name of the test that was executed
         */
        static void printTestFooter(const std::string& testName);
        
        /**
         * @brief Test basic error reporting functionality
         * 
         * Verifies:
         * - Simple error message reporting
         * - Error flag setting
         * - Error count tracking
         * - Basic error retrieval
         */
        static void testBasicErrorReporting();
        
        /**
         * @brief Test detailed error messages
         * 
         * Verifies:
         * - Error type specification
         * - Detailed error descriptions
         * - Message and details separation
         * - Error information completeness
         */
        static void testDetailedErrorMessages();
        
        /**
         * @brief Test error type classification
         * 
         * Verifies:
         * - Different error types (UnexpectedToken, MissingToken, etc.)
         * - Proper error type assignment
         * - Error type retrieval
         * - Multiple error types handling
         */
        static void testErrorTypeClassification();
        
        /**
         * @brief Test error location tracking
         * 
         * Verifies:
         * - Source location capture
         * - Location information accuracy
         * - Token-based location tracking
         * - Location validity
         */
        static void testErrorLocationTracking();
        
        /**
         * @brief Test multiple error handling
         * 
         * Verifies:
         * - Multiple error accumulation
         * - Error count accuracy
         * - Error order preservation
         * - Error collection integrity
         */
        static void testMultipleErrorHandling();
        
        /**
         * @brief Test error recovery mechanisms
         * 
         * Verifies:
         * - Error clearing functionality
         * - State reset after error clearing
         * - Error flag reset
         * - Clean state restoration
         */
        static void testErrorRecovery();
        
        /**
         * @brief Test consume method error integration
         * 
         * Verifies:
         * - Enhanced consume() method error reporting
         * - Detailed error messages in consume()
         * - Error type classification in consume()
         * - Token expectation error handling
         */
        static void testConsumeMethodErrors();
        
        /**
         * @brief Test lexical error handling
         * 
         * Verifies:
         * - Lexical error detection during advance()
         * - Proper error type for lexical errors
         * - Error message formatting for lexical errors
         * - Integration with lexer error reporting
         */
        static void testLexicalErrorHandling();
    };
}

#endif // ENHANCED_PARSER_ERROR_TEST_HPP