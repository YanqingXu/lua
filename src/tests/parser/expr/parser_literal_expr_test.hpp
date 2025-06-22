#ifndef LITERAL_EXPR_TEST_HPP
#define LITERAL_EXPR_TEST_HPP

#include <iostream>
#include "../../../parser/parser.hpp"
#include "../../../parser/ast/expressions.hpp"
#include "../../../test_framework/core/test_macros.hpp"

namespace Lua {
namespace Tests {
namespace Parser {
namespace Expr {

/**
 * @brief Literal Expression Parser Test Class
 * 
 * Namespace: Lua::Tests::Parser::Expr
 * 
 * Tests parsing of literal expressions including:
 * - Number literals (integers, floats)
 * - String literals (single/double quotes, escape sequences)
 * - Boolean literals (true, false)
 * - Nil literal
 * - Special number formats (hex, scientific notation)
 */
class LiteralExprTest {
public:
    /**
     * @brief Run all literal expression tests
     * 
     * Executes all test cases for literal expression parsing
     */
    static void runAllTests();
    
private:
    // Number literal tests
    static void testIntegerLiterals();
    static void testFloatLiterals();
    static void testScientificNotation();
    static void testHexadecimalNumbers();
    
    // String literal tests
    static void testSingleQuoteStrings();
    static void testDoubleQuoteStrings();
    static void testLongStrings();
    static void testEscapeSequences();
    static void testEmptyStrings();
    
    // Boolean and nil literal tests
    static void testBooleanLiterals();
    static void testNilLiteral();
    
    // Error handling tests
    static void testInvalidNumberFormats();
    static void testUnterminatedStrings();
    static void testInvalidEscapeSequences();
    
    // Helper methods
    static void testLiteralParsing(const std::string& input, const std::string& testName);
    static void testLiteralParsingError(const std::string& input, const std::string& testName);
    //static bool verifyLiteralValue(const Expr* expr, const Value& expectedValue);
};

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua

#endif // LITERAL_EXPR_TEST_HPP