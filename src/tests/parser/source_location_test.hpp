#ifndef SOURCE_LOCATION_TEST_HPP
#define SOURCE_LOCATION_TEST_HPP

#include <iostream>
#include <string>
#include "../../parser/ast/source_location.hpp"
#include "../../lexer/lexer.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief SourceLocation Test Class
 * 
 * This class provides comprehensive tests for the SourceLocation and SourceRange classes.
 * It tests basic functionality, comparison operations, formatting, and integration with tokens.
 */
class SourceLocationTest {
public:
    /**
     * @brief Run all SourceLocation tests
     * 
     * Executes all test cases for SourceLocation functionality.
     */
    static void runAllTests();
    
private:
    /**
     * @brief Test basic SourceLocation construction and properties
     */
    static void testBasicConstruction();
    
    /**
     * @brief Test SourceLocation creation from line and column
     */
    static void testFromLineColumn();
    
    /**
     * @brief Test SourceLocation creation from Token
     */
    static void testFromToken();
    
    /**
     * @brief Test SourceLocation formatting and string representation
     */
    static void testFormatting();
    
    /**
     * @brief Test SourceLocation comparison operations
     */
    static void testComparison();
    
    /**
     * @brief Test SourceRange functionality
     */
    static void testSourceRange();
    
    /**
     * @brief Test SourceLocation setter methods
     */
    static void testSetters();
    
    /**
     * @brief Test SourceLocation with AST nodes integration
     */
    static void testASTIntegration();
    
    /**
     * @brief Helper function to print test results
     * @param testName Name of the test
     * @param passed Whether the test passed
     */
    static void printTestResult(const std::string& testName, bool passed);
    
    /**
     * @brief Helper function to print section header
     * @param sectionName Name of the test section
     */
    static void printSectionHeader(const std::string& sectionName);
};

} // namespace Tests
} // namespace Lua

#endif // SOURCE_LOCATION_TEST_HPP