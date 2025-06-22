#include "unary_expr_test.hpp"

namespace Lua {
namespace Tests {
namespace Parser {
namespace Expr {

void UnaryExprTest::runAllTests() {
    // Arithmetic unary operator tests
    RUN_TEST(UnaryExprTest, testUnaryMinus);
    RUN_TEST(UnaryExprTest, testUnaryPlus);
    
    // Logical unary operator tests
    RUN_TEST(UnaryExprTest, testLogicalNot);
    
    // Length operator tests
    RUN_TEST(UnaryExprTest, testLengthOperator);
    
    // Precedence and associativity tests
    RUN_TEST(UnaryExprTest, testUnaryPrecedence);
    RUN_TEST(UnaryExprTest, testNestedUnaryExpressions);
    RUN_TEST(UnaryExprTest, testUnaryWithLiterals);
    RUN_TEST(UnaryExprTest, testUnaryWithVariables);
    
    // Complex expression tests
    RUN_TEST(UnaryExprTest, testUnaryInBinaryExpressions);
    RUN_TEST(UnaryExprTest, testMultipleUnaryOperators);
    
    // Error handling tests
    RUN_TEST(UnaryExprTest, testInvalidUnaryOperators);
    RUN_TEST(UnaryExprTest, testMissingOperand);
}

void UnaryExprTest::testUnaryMinus() {
    testUnaryParsing("-5", TokenType::MINUS, "Simple unary minus with number");
    testUnaryParsing("-x", TokenType::MINUS, "Unary minus with variable");
    testUnaryParsing("-3.14", TokenType::MINUS, "Unary minus with float");
    testUnaryParsing("-0x10", TokenType::MINUS, "Unary minus with hexadecimal");
}

void UnaryExprTest::testUnaryPlus() {
    testUnaryParsing("+5", TokenType::PLUS, "Simple unary plus with number");
    testUnaryParsing("+x", TokenType::PLUS, "Unary plus with variable");
    testUnaryParsing("+3.14", TokenType::PLUS, "Unary plus with float");
    testUnaryParsing("+0xFF", TokenType::PLUS, "Unary plus with hexadecimal");
}

void UnaryExprTest::testLogicalNot() {
    testUnaryParsing("not true", TokenType::NOT, "Logical not with boolean");
    testUnaryParsing("not x", TokenType::NOT, "Logical not with variable");
    testUnaryParsing("not nil", TokenType::NOT, "Logical not with nil");
    testUnaryParsing("not 0", TokenType::NOT, "Logical not with number");
}

void UnaryExprTest::testLengthOperator() {
    testUnaryParsing("#t", TokenType::HASH, "Length operator with table");
    testUnaryParsing("#\"hello\"", TokenType::HASH, "Length operator with string");
    testUnaryParsing("#arr", TokenType::HASH, "Length operator with array variable");
}

void UnaryExprTest::testUnaryPrecedence() {
    testUnaryParsing("-x + y", TokenType::MINUS, "Unary minus precedence with addition");
    testUnaryParsing("not x and y", TokenType::NOT, "Logical not precedence with and");
    testUnaryParsing("#t * 2", TokenType::HASH, "Length operator precedence with multiplication");
}

void UnaryExprTest::testNestedUnaryExpressions() {
    testUnaryParsing("--x", TokenType::MINUS, "Double unary minus");
    testUnaryParsing("not not x", TokenType::NOT, "Double logical not");
    testUnaryParsing("-+x", TokenType::MINUS, "Unary minus and plus combination");
    testUnaryParsing("+-x", TokenType::PLUS, "Unary plus and minus combination");
    testUnaryParsing("not -x", TokenType::NOT, "Logical not with unary minus");
    testUnaryParsing("-not x", TokenType::MINUS, "Unary minus with logical not");
}

void UnaryExprTest::testUnaryWithLiterals() {
    testUnaryParsing("-42", TokenType::MINUS, "Unary minus with integer");
    testUnaryParsing("+3.14159", TokenType::PLUS, "Unary plus with float");
    testUnaryParsing("not false", TokenType::NOT, "Logical not with boolean false");
    testUnaryParsing("not true", TokenType::NOT, "Logical not with boolean true");
    testUnaryParsing("#\"test\"", TokenType::HASH, "Length operator with string literal");
    testUnaryParsing("-0", TokenType::MINUS, "Unary minus with zero");
}

void UnaryExprTest::testUnaryWithVariables() {
    testUnaryParsing("-variable", TokenType::MINUS, "Unary minus with simple variable");
    testUnaryParsing("+count", TokenType::PLUS, "Unary plus with variable");
    testUnaryParsing("not flag", TokenType::NOT, "Logical not with variable");
    testUnaryParsing("#array", TokenType::HASH, "Length operator with variable");
    testUnaryParsing("-_private", TokenType::MINUS, "Unary minus with underscore variable");
    testUnaryParsing("not isValid", TokenType::NOT, "Logical not with camelCase variable");
}

void UnaryExprTest::testUnaryInBinaryExpressions() {
    // Note: These tests focus on the unary part, binary parsing is tested separately
    testUnaryParsing("-a + b", TokenType::MINUS, "Unary minus in addition");
    testUnaryParsing("not a or b", TokenType::NOT, "Logical not in or expression");
    testUnaryParsing("#a == 5", TokenType::HASH, "Length operator in comparison");
    testUnaryParsing("+x * y", TokenType::PLUS, "Unary plus in multiplication");
}

void UnaryExprTest::testMultipleUnaryOperators() {
    testUnaryParsing("---x", TokenType::MINUS, "Triple unary minus");
    testUnaryParsing("not not not x", TokenType::NOT, "Triple logical not");
    testUnaryParsing("-+-x", TokenType::MINUS, "Complex unary combination");
    testUnaryParsing("not -+x", TokenType::NOT, "Logical not with arithmetic unary");
}

void UnaryExprTest::testInvalidUnaryOperators() {
    testUnaryParsingError("*x", "Invalid unary operator *");
    testUnaryParsingError("/x", "Invalid unary operator /");
    testUnaryParsingError("%x", "Invalid unary operator %");
    testUnaryParsingError("&x", "Invalid unary operator &");
    testUnaryParsingError("|x", "Invalid unary operator |");
}

void UnaryExprTest::testMissingOperand() {
    testUnaryParsingError("-", "Unary minus without operand");
    testUnaryParsingError("+", "Unary plus without operand");
    testUnaryParsingError("not", "Logical not without operand");
    testUnaryParsingError("#", "Length operator without operand");
    testUnaryParsingError("- +", "Unary operators without operand");
}

void UnaryExprTest::testUnaryParsing(const std::string& input, TokenType expectedOp, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printTestResult(testName, false, "Failed to parse expression");
            return;
        }
        
        if (!verifyUnaryExpression(expr.get(), expectedOp)) {
            TestUtils::printTestResult(testName, false, "Expression is not a unary expression or operator mismatch");
            return;
        }
        
        TestUtils::printTestResult(testName, true, "Successfully parsed unary expression");
        
        // Print additional info for debugging
        if (auto unaryExpr = dynamic_cast<const UnaryExpr*>(expr.get())) {
            printUnaryExpressionInfo(unaryExpr);
        }
        
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, false, std::string("Exception: ") + e.what());
    }
}

void UnaryExprTest::testUnaryParsingError(const std::string& input, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printTestResult(testName, true, "Correctly failed to parse invalid unary expression");
            return;
        }
        
        TestUtils::printTestResult(testName, false, "Should have failed to parse invalid unary expression");
        
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, true, std::string("Correctly threw exception: ") + e.what());
    }
}

bool UnaryExprTest::verifyUnaryExpression(const Expr* expr, TokenType expectedOp) {
    if (!expr) {
        return false;
    }
    
    const UnaryExpr* unaryExpr = dynamic_cast<const UnaryExpr*>(expr);
    if (!unaryExpr) {
        return false;
    }
    
    return unaryExpr->op.type == expectedOp;
}

void UnaryExprTest::printUnaryExpressionInfo(const UnaryExpr* unaryExpr) {
    if (!unaryExpr) {
        return;
    }
    
    std::string opStr;
    switch (unaryExpr->op.type) {
        case TokenType::MINUS: opStr = "-"; break;
        case TokenType::PLUS: opStr = "+"; break;
        case TokenType::NOT: opStr = "not"; break;
        case TokenType::HASH: opStr = "#"; break;
        default: opStr = "unknown"; break;
    }
    
    TestUtils::printInfo("  Operator: " + opStr);
    TestUtils::printInfo("  Has operand: " + std::string(unaryExpr->operand ? "yes" : "no"));
}

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua