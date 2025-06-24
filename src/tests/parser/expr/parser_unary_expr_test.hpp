#ifndef UNARY_EXPR_TEST_HPP
#define UNARY_EXPR_TEST_HPP

#include <string>
#include "../../../parser/ast/expressions.hpp"

namespace Lua {
namespace Tests {
/**
 * @brief Unary Expression Parser Test Class
 * 
 * Namespace: Lua::Tests::Parser::Expr
 * 
 * Tests parsing of unary expressions including:
 * - Arithmetic unary operators (-, +)
 * - Logical unary operator (not)
 * - Length operator (#)
 * - Operator precedence with binary expressions
 * - Nested unary expressions
 * - Complex combinations
 */
class ParserUnaryExprTest {
public:
    /**
     * @brief Run all unary expression tests
     * 
     * Executes all test cases for unary expression parsing
     */
    static void runAllTests();
    
private:
    // Arithmetic unary operator tests
    static void testUnaryMinus();
    static void testUnaryPlus();
    
    // Logical unary operator tests
    static void testLogicalNot();
    
    // Length operator tests
    static void testLengthOperator();
    
    // Precedence and associativity tests
    static void testUnaryPrecedence();
    static void testNestedUnaryExpressions();
    static void testUnaryWithLiterals();
    static void testUnaryWithVariables();
    
    // Complex expression tests
    static void testUnaryInBinaryExpressions();
    static void testMultipleUnaryOperators();
    
    // Error handling tests
    static void testInvalidUnaryOperators();
    static void testMissingOperand();
    
    // Helper methods
    static void testUnaryParsing(const std::string& input, TokenType expectedOp, const std::string& testName);
    static void testUnaryParsingError(const std::string& input, const std::string& testName);
    static bool verifyUnaryExpression(const Expr* expr, TokenType expectedOp);
    static void printUnaryExpressionInfo(const UnaryExpr* unaryExpr);
};
} // namespace Tests
} // namespace Lua

#endif // UNARY_EXPR_TEST_HPP