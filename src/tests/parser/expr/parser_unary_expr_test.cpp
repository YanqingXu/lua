#include "parser_unary_expr_test.hpp"

#include "../../../parser/parser.hpp"
#include "../../../parser/ast/expressions.hpp"
#include "../../../test_framework/core/test_macros.hpp"
#include "../../../test_framework/core/test_utils.hpp"

using namespace Lua;
using namespace Tests;
using namespace TestFramework;

void ParserUnaryExprTest::runAllTests() {
    // Arithmetic unary operator tests
    RUN_TEST(ParserUnaryExprTest, testUnaryMinus);
    RUN_TEST(ParserUnaryExprTest, testUnaryPlus);
    
    // Logical unary operator tests
    RUN_TEST(ParserUnaryExprTest, testLogicalNot);
    
    // Length operator tests
    RUN_TEST(ParserUnaryExprTest, testLengthOperator);
    
    // Precedence and associativity tests
    RUN_TEST(ParserUnaryExprTest, testUnaryPrecedence);
    RUN_TEST(ParserUnaryExprTest, testNestedUnaryExpressions);
    RUN_TEST(ParserUnaryExprTest, testUnaryWithLiterals);
    RUN_TEST(ParserUnaryExprTest, testUnaryWithVariables);
    
    // Complex expression tests
    RUN_TEST(ParserUnaryExprTest, testUnaryInBinaryExpressions);
    RUN_TEST(ParserUnaryExprTest, testMultipleUnaryOperators);
    
    // Error handling tests
    RUN_TEST(ParserUnaryExprTest, testInvalidUnaryOperators);
    RUN_TEST(ParserUnaryExprTest, testMissingOperand);
}

void ParserUnaryExprTest::testUnaryMinus() {
    testUnaryParsing("-5", TokenType::Minus, "Simple unary minus with number");
    testUnaryParsing("-x", TokenType::Minus, "Unary minus with variable");
    testUnaryParsing("-3.14", TokenType::Minus, "Unary minus with float");
    testUnaryParsing("-0x10", TokenType::Minus, "Unary minus with hexadecimal");
}

void ParserUnaryExprTest::testUnaryPlus() {
    testUnaryParsing("+5", TokenType::Plus, "Simple unary plus with number");
    testUnaryParsing("+x", TokenType::Plus, "Unary plus with variable");
    testUnaryParsing("+3.14", TokenType::Plus, "Unary plus with float");
    testUnaryParsing("+0xFF", TokenType::Plus, "Unary plus with hexadecimal");
}

void ParserUnaryExprTest::testLogicalNot() {
    testUnaryParsing("not true", TokenType::Not, "Logical not with boolean");
    testUnaryParsing("not x", TokenType::Not, "Logical not with variable");
    testUnaryParsing("not nil", TokenType::Not, "Logical not with nil");
    testUnaryParsing("not 0", TokenType::Not, "Logical not with number");
}

void ParserUnaryExprTest::testLengthOperator() {
    testUnaryParsing("#t", TokenType::Hash, "Length operator with table");
    testUnaryParsing("#\"hello\"", TokenType::Hash, "Length operator with string");
    testUnaryParsing("#arr", TokenType::Hash, "Length operator with array variable");
}

void ParserUnaryExprTest::testUnaryPrecedence() {
    testUnaryParsing("-x + y", TokenType::Minus, "Unary minus precedence with addition");
    testUnaryParsing("not x and y", TokenType::Not, "Logical not precedence with and");
    testUnaryParsing("#t * 2", TokenType::Hash, "Length operator precedence with multiplication");
}

void ParserUnaryExprTest::testNestedUnaryExpressions() {
    testUnaryParsing("--x", TokenType::Minus, "Double unary minus");
    testUnaryParsing("not not x", TokenType::Not, "Double logical not");
    testUnaryParsing("-+x", TokenType::Minus, "Unary minus and plus combination");
    testUnaryParsing("+-x", TokenType::Plus, "Unary plus and minus combination");
    testUnaryParsing("not -x", TokenType::Not, "Logical not with unary minus");
    testUnaryParsing("-not x", TokenType::Minus, "Unary minus with logical not");
}

void ParserUnaryExprTest::testUnaryWithLiterals() {
    testUnaryParsing("-42", TokenType::Minus, "Unary minus with integer");
    testUnaryParsing("+3.14159", TokenType::Plus, "Unary plus with float");
    testUnaryParsing("not false", TokenType::Not, "Logical not with boolean false");
    testUnaryParsing("not true", TokenType::Not, "Logical not with boolean true");
    testUnaryParsing("#\"test\"", TokenType::Hash, "Length operator with string literal");
    testUnaryParsing("-0", TokenType::Minus, "Unary minus with zero");
}

void ParserUnaryExprTest::testUnaryWithVariables() {
    testUnaryParsing("-variable", TokenType::Minus, "Unary minus with simple variable");
    testUnaryParsing("+count", TokenType::Plus, "Unary plus with variable");
    testUnaryParsing("not flag", TokenType::Not, "Logical not with variable");
    testUnaryParsing("#array", TokenType::Hash, "Length operator with variable");
    testUnaryParsing("-_private", TokenType::Minus, "Unary minus with underscore variable");
    testUnaryParsing("not isValid", TokenType::Not, "Logical not with camelCase variable");
}

void ParserUnaryExprTest::testUnaryInBinaryExpressions() {
    // Note: These tests focus on the unary part, binary parsing is tested separately
    testUnaryParsing("-a + b", TokenType::Minus, "Unary minus in addition");
    testUnaryParsing("not a or b", TokenType::Not, "Logical not in or expression");
    testUnaryParsing("#a == 5", TokenType::Hash, "Length operator in comparison");
    testUnaryParsing("+x * y", TokenType::Plus, "Unary plus in multiplication");
}

void ParserUnaryExprTest::testMultipleUnaryOperators() {
    testUnaryParsing("---x", TokenType::Minus, "Triple unary minus");
    testUnaryParsing("not not not x", TokenType::Not, "Triple logical not");
    testUnaryParsing("-+-x", TokenType::Minus, "Complex unary combination");
    testUnaryParsing("not -+x", TokenType::Not, "Logical not with arithmetic unary");
}

void ParserUnaryExprTest::testInvalidUnaryOperators() {
    testUnaryParsingError("*x", "Invalid unary operator *");
    testUnaryParsingError("/x", "Invalid unary operator /");
    testUnaryParsingError("%x", "Invalid unary operator %");
    testUnaryParsingError("&x", "Invalid unary operator &");
    testUnaryParsingError("|x", "Invalid unary operator |");
}

void ParserUnaryExprTest::testMissingOperand() {
    testUnaryParsingError("-", "Unary minus without operand");
    testUnaryParsingError("+", "Unary plus without operand");
    testUnaryParsingError("not", "Logical not without operand");
    testUnaryParsingError("#", "Length operator without operand");
    testUnaryParsingError("- +", "Unary operators without operand");
}

void ParserUnaryExprTest::testUnaryParsing(const std::string& input, TokenType expectedOp, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printTestResult(testName, false);
            TestUtils::printInfo("Failed to parse expression");
            return;
        }
        
        if (!verifyUnaryExpression(expr.get(), expectedOp)) {
            TestUtils::printTestResult(testName, false);
            TestUtils::printInfo("Expression is not a unary expression or operator mismatch");
            return;
        }
        
        TestUtils::printTestResult(testName, true);
        TestUtils::printInfo("Successfully parsed unary expression");
        
        // Print additional info for debugging
        if (auto unaryExpr = dynamic_cast<const UnaryExpr*>(expr.get())) {
            printUnaryExpressionInfo(unaryExpr);
        }
        
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, false);
        TestUtils::printInfo(std::string("Exception: ") + e.what());
    }
}

void ParserUnaryExprTest::testUnaryParsingError(const std::string& input, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printTestResult(testName, true);
            TestUtils::printInfo("Correctly failed to parse invalid unary expression");
            return;
        }
        
        TestUtils::printTestResult(testName, false);
        TestUtils::printInfo("Should have failed to parse invalid unary expression");
        
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, true);
        TestUtils::printInfo(std::string("Correctly threw exception: ") + e.what());
    }
}

bool ParserUnaryExprTest::verifyUnaryExpression(const Expr* expr, TokenType expectedOp) {
    if (!expr) {
        return false;
    }
    
    const UnaryExpr* unaryExpr = dynamic_cast<const UnaryExpr*>(expr);
    if (!unaryExpr) {
        return false;
    }
    
    return unaryExpr->getOperator() == expectedOp;
}

void ParserUnaryExprTest::printUnaryExpressionInfo(const UnaryExpr* unaryExpr) {
    if (!unaryExpr) {
        return;
    }
    
    std::string opStr;
    switch (unaryExpr->getOperator()) {
        case TokenType::Minus: opStr = "-"; break;
        case TokenType::Plus: opStr = "+"; break;
        case TokenType::Not: opStr = "not"; break;
        case TokenType::Hash: opStr = "#"; break;
        default: opStr = "unknown"; break;
    }
    
    TestUtils::printInfo("  Operator: " + opStr);
    TestUtils::printInfo("  Has operand: " + std::string(unaryExpr->getRight() ? "yes" : "no"));
}