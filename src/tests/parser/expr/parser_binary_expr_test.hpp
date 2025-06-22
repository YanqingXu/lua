#ifndef BINARY_EXPR_TEST_HPP
#define BINARY_EXPR_TEST_HPP

#include <iostream>
#include "../../../parser/ast/ast_base.hpp"
#include "../../../parser/parser.hpp"
#include "../../../parser/ast/expressions.hpp"
#include "../../../test_framework/core/test_macros.hpp"

namespace Lua {
namespace Tests {
namespace Parser {
namespace Expr {
/**
 * @brief Binary Expression Parser Test Class
 * 
 * Namespace: Lua::Tests::Parser::Expr
 * 
 * Tests parsing of binary expressions including:
 * - Arithmetic operators (+, -, *, /, %, ^)
 * - Comparison operators (==, ~=, <, <=, >, >=)
 * - Logical operators (and, or)
 * - String concatenation (..)
 * - Operator precedence and associativity
 * - Complex nested expressions
 */
class BinaryExprTest {
public:
    /**
     * @brief Run all binary expression tests
     * 
     * Executes all test cases for binary expression parsing
     */
    static void runAllTests();
    
private:
    // Arithmetic operator tests
    static void testArithmeticOperators();
    static void testAddition();
    static void testSubtraction();
    static void testMultiplication();
    static void testDivision();
    static void testModulo();
    static void testExponentiation();
    
    // Comparison operator tests
    static void testComparisonOperators();
    static void testEquality();
    static void testInequality();
    static void testLessThan();
    static void testLessEqual();
    static void testGreaterThan();
    static void testGreaterEqual();
    
    // Logical operator tests
    static void testLogicalOperators();
    static void testLogicalAnd();
    static void testLogicalOr();
    
    // String concatenation tests
    static void testStringConcatenation();
    
    // Precedence and associativity tests
    static void testOperatorPrecedence();
    static void testLeftAssociativity();
    static void testRightAssociativity();
    static void testMixedPrecedence();
    
    // Complex expression tests
    static void testNestedExpressions();
    static void testParenthesizedExpressions();
    static void testComplexArithmetic();
    static void testComplexLogical();
    static void testMixedOperatorTypes();
    
    // Edge case tests
    static void testWithLiterals();
    static void testWithVariables();
    static void testWithUnaryExpressions();
    static void testChainedComparisons();
    
    // Error handling tests
    static void testInvalidOperators();
    static void testMissingOperands();
    static void testInvalidSyntax();
    
    // Helper methods
    static void testBinaryParsing(const std::string& input, TokenType expectedOp, const std::string& testName);
    static void testBinaryParsingError(const std::string& input, const std::string& testName);
    //static bool verifyBinaryExpression(const Expr* expr, TokenType expectedOp);
    static void printBinaryExpressionInfo(const BinaryExpr* binaryExpr);
    static std::string tokenTypeToString(TokenType type);
};

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua

#endif // BINARY_EXPR_TEST_HPP