#ifndef VARIABLE_EXPR_TEST_HPP
#define VARIABLE_EXPR_TEST_HPP

#include <iostream>
#include "../../../parser/parser.hpp"
#include "../../../parser/ast/ast_base.hpp"
#include "../../../parser/ast/expressions.hpp"
#include "../../../test_framework/core/test_macros.hpp"

namespace Lua {
namespace Tests {
namespace Parser {
namespace Expr {

/**
 * @brief Variable Expression Parser Test Class
 * 
 * Namespace: Lua::Tests::Parser::Expr
 * 
 * Tests parsing of variable expressions including:
 * - Simple variable names
 * - Variable names with underscores
 * - Variable names with numbers
 * - Reserved keyword handling
 * - Invalid variable name detection
 */
class VariableExprTest {
public:
    /**
     * @brief Run all variable expression tests
     * 
     * Executes all test cases for variable expression parsing
     */
    static void runAllTests();
    
private:
    // Basic variable tests
    static void testSimpleVariables();
    static void testVariableNaming();
    static void testUnicodeVariables();
    
    // Variable validation tests
    static void testValidIdentifiers();
    static void testInvalidIdentifiers();
    static void testReservedKeywords();
    
    // Scope-related tests
    static void testGlobalVariables();
    static void testLocalVariables();
    
    // Edge case tests
    static void testLongVariableNames();
    static void testVariableWithNumbers();
    static void testVariableWithUnderscores();
    
    // Error handling tests
    static void testInvalidVariableNames();
    static void testKeywordAsVariable();
    
    // Helper methods
    static void testVariableParsing(const std::string& input, const std::string& expectedName, const std::string& testName);
    static void testVariableParsingError(const std::string& input, const std::string& testName);
    static bool verifyVariableName(const Lua::Expr* expr, const std::string& expectedName);
};

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua

#endif // VARIABLE_EXPR_TEST_HPP