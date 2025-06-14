#ifndef ERROR_REPORTER_TEST_HPP
#define ERROR_REPORTER_TEST_HPP

#include "../../parser/ast/parse_error.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief ErrorReporter Test Suite
 * 
 * This class provides comprehensive tests for the ErrorReporter functionality,
 * including error reporting, filtering, output formats, and integration with ParseError.
 */
class ErrorReporterTest {
public:
    /**
     * @brief Run all ErrorReporter tests
     * 
     * Executes all ErrorReporter-related test cases in a logical order.
     * Tests cover basic functionality, configuration options, and output formats.
     */
    static void runAllTests();
    
private:
    /**
     * @brief Test basic error reporting functionality
     * 
     * Tests:
     * - Basic error and warning reporting
     * - Error counting
     * - hasErrors() functionality
     */
    static void testBasicErrorReporting();
    
    /**
     * @brief Test error filtering based on configuration
     * 
     * Tests:
     * - Filtering warnings and info messages
     * - Severity-based filtering
     * - Configuration options
     */
    static void testErrorFiltering();
    
    /**
     * @brief Test convenience methods for common error types
     * 
     * Tests:
     * - reportUnexpectedToken()
     * - reportMissingToken()
     * - reportSemanticError()
     */
    static void testConvenienceMethods();
    
    /**
     * @brief Test different output formats
     * 
     * Tests:
     * - toString()
     * - toDetailedString()
     * - toShortString()
     * - toJson()
     */
    static void testOutputFormats();
    
    /**
     * @brief Test static factory methods
     * 
     * Tests:
     * - createDefault()
     * - createStrict()
     * - createPermissive()
     */
    static void testStaticFactoryMethods();
    
    /**
     * @brief Test maximum errors limit functionality
     * 
     * Tests:
     * - Error limit enforcement
     * - shouldStopParsing() behavior
     * - Configuration of max errors
     */
    static void testMaxErrorsLimit();
    
    /**
     * @brief Test error clearing functionality
     * 
     * Tests:
     * - clear() method
     * - State reset after clearing
     */
    static void testErrorClear();
    
    /**
     * @brief Test integration with ParseError class
     * 
     * Tests:
     * - ParseError object creation
     * - Error details preservation
     * - Severity handling
     */
    static void testIntegrationWithParseError();
    
    /**
     * @brief Test JSON output format
     * 
     * Tests:
     * - JSON structure validity
     * - Error serialization
     * - Metadata inclusion
     */
    static void testJsonOutput();
    
    /**
     * @brief Test error severity handling
     * 
     * Tests:
     * - Different severity levels
     * - Severity-based counting
     * - Severity filtering
     */
    static void testErrorSeverityHandling();
    
    /**
     * @brief Print section header for test organization
     * @param testName Name of the test being executed
     */
    static void printTestHeader(const std::string& testName);
    
    /**
     * @brief Print test completion message
     * @param testName Name of the completed test
     */
    static void printTestFooter(const std::string& testName);
};

} // namespace Tests
} // namespace Lua

#endif // ERROR_REPORTER_TEST_HPP