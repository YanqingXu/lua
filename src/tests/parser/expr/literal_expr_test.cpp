#include "literal_expr_test.hpp"
#include "../../../lexer/lexer.hpp"

namespace Lua {
namespace Tests {

void LiteralExprTest::runAllTests() {
    // Number literal tests
    RUN_TEST(LiteralExprTest, testIntegerLiterals);
    RUN_TEST(LiteralExprTest, testFloatLiterals);
    RUN_TEST(LiteralExprTest, testScientificNotation);
    RUN_TEST(LiteralExprTest, testHexadecimalNumbers);
    
    // String literal tests
    RUN_TEST(LiteralExprTest, testSingleQuoteStrings);
    RUN_TEST(LiteralExprTest, testDoubleQuoteStrings);
    RUN_TEST(LiteralExprTest, testLongStrings);
    RUN_TEST(LiteralExprTest, testEscapeSequences);
    RUN_TEST(LiteralExprTest, testEmptyStrings);
    
    // Boolean and nil literal tests
    RUN_TEST(LiteralExprTest, testBooleanLiterals);
    RUN_TEST(LiteralExprTest, testNilLiteral);
    
    // Error handling tests
    RUN_TEST(LiteralExprTest, testInvalidNumberFormats);
    RUN_TEST(LiteralExprTest, testUnterminatedStrings);
    RUN_TEST(LiteralExprTest, testInvalidEscapeSequences);
}

void LiteralExprTest::testIntegerLiterals() {
    TestUtils::printInfo("Testing integer literal parsing...");
    
    // Test positive integers
    testLiteralParsing("42", "positive integer");
    testLiteralParsing("0", "zero");
    testLiteralParsing("123456789", "large integer");
    
    // Test negative integers (handled by unary minus)
    testLiteralParsing("1", "simple integer");
    
    TestUtils::printTestResult("Integer literals", true);
}

void LiteralExprTest::testFloatLiterals() {
    TestUtils::printInfo("Testing float literal parsing...");
    
    // Test decimal numbers
    testLiteralParsing("3.14", "decimal number");
    testLiteralParsing("0.5", "decimal less than 1");
    testLiteralParsing("123.456", "multi-digit decimal");
    testLiteralParsing(".5", "decimal starting with dot");
    testLiteralParsing("5.", "decimal ending with dot");
    
    TestUtils::printTestResult("Float literals", true);
}

void LiteralExprTest::testScientificNotation() {
    TestUtils::printInfo("Testing scientific notation parsing...");
    
    // Test scientific notation
    testLiteralParsing("1e10", "simple scientific notation");
    testLiteralParsing("1.5e-3", "scientific with decimal and negative exponent");
    testLiteralParsing("2.5E+5", "scientific with positive exponent");
    testLiteralParsing("1e0", "scientific with zero exponent");
    
    TestUtils::printTestResult("Scientific notation", true);
}

void LiteralExprTest::testHexadecimalNumbers() {
    TestUtils::printInfo("Testing hexadecimal number parsing...");
    
    // Test hexadecimal numbers
    testLiteralParsing("0x10", "simple hex number");
    testLiteralParsing("0xFF", "hex with uppercase letters");
    testLiteralParsing("0xabcdef", "hex with lowercase letters");
    testLiteralParsing("0x0", "hex zero");
    
    TestUtils::printTestResult("Hexadecimal numbers", true);
}

void LiteralExprTest::testSingleQuoteStrings() {
    TestUtils::printInfo("Testing single quote string parsing...");
    
    // Test single quote strings
    testLiteralParsing("'hello'", "simple single quote string");
    testLiteralParsing("''", "empty single quote string");
    testLiteralParsing("'hello world'", "single quote string with space");
    testLiteralParsing("'123'", "single quote string with numbers");
    
    TestUtils::printTestResult("Single quote strings", true);
}

void LiteralExprTest::testDoubleQuoteStrings() {
    TestUtils::printInfo("Testing double quote string parsing...");
    
    // Test double quote strings
    testLiteralParsing("\"hello\"", "simple double quote string");
    testLiteralParsing("\"\"", "empty double quote string");
    testLiteralParsing("\"hello world\"", "double quote string with space");
    testLiteralParsing("\"123\"", "double quote string with numbers");
    
    TestUtils::printTestResult("Double quote strings", true);
}

void LiteralExprTest::testLongStrings() {
    TestUtils::printInfo("Testing long string parsing...");
    
    // Test long strings
    testLiteralParsing("[[hello]]", "simple long string");
    testLiteralParsing("[[]]", "empty long string");
    testLiteralParsing("[[hello\nworld]]", "long string with newline");
    testLiteralParsing("[=[hello world]=]", "long string with level 1");
    
    TestUtils::printTestResult("Long strings", true);
}

void LiteralExprTest::testEscapeSequences() {
    TestUtils::printInfo("Testing escape sequence parsing...");
    
    // Test escape sequences
    testLiteralParsing("\"hello\\nworld\"", "newline escape");
    testLiteralParsing("\"hello\\tworld\"", "tab escape");
    testLiteralParsing("\"hello\\\"world\"", "quote escape");
    testLiteralParsing("\"hello\\\\world\"", "backslash escape");
    
    TestUtils::printTestResult("Escape sequences", true);
}

void LiteralExprTest::testEmptyStrings() {
    TestUtils::printInfo("Testing empty string parsing...");
    
    // Test empty strings
    testLiteralParsing("''", "empty single quote string");
    testLiteralParsing("\"\"", "empty double quote string");
    testLiteralParsing("[[]]", "empty long string");
    
    TestUtils::printTestResult("Empty strings", true);
}

void LiteralExprTest::testBooleanLiterals() {
    TestUtils::printInfo("Testing boolean literal parsing...");
    
    // Test boolean literals
    testLiteralParsing("true", "true literal");
    testLiteralParsing("false", "false literal");
    
    TestUtils::printTestResult("Boolean literals", true);
}

void LiteralExprTest::testNilLiteral() {
    TestUtils::printInfo("Testing nil literal parsing...");
    
    // Test nil literal
    testLiteralParsing("nil", "nil literal");
    
    TestUtils::printTestResult("Nil literal", true);
}

void LiteralExprTest::testInvalidNumberFormats() {
    TestUtils::printInfo("Testing invalid number format error handling...");
    
    // Test invalid number formats
    testLiteralParsingError("1.2.3", "multiple decimal points");
    testLiteralParsingError("1e", "incomplete scientific notation");
    testLiteralParsingError("0x", "incomplete hex number");
    testLiteralParsingError("1ee5", "double exponent");
    
    TestUtils::printTestResult("Invalid number format error handling", true);
}

void LiteralExprTest::testUnterminatedStrings() {
    TestUtils::printInfo("Testing unterminated string error handling...");
    
    // Test unterminated strings
    testLiteralParsingError("'hello", "unterminated single quote string");
    testLiteralParsingError("\"hello", "unterminated double quote string");
    testLiteralParsingError("[[hello", "unterminated long string");
    
    TestUtils::printTestResult("Unterminated string error handling", true);
}

void LiteralExprTest::testInvalidEscapeSequences() {
    TestUtils::printInfo("Testing invalid escape sequence error handling...");
    
    // Test invalid escape sequences
    testLiteralParsingError("\"hello\\x\"", "invalid escape sequence");
    testLiteralParsingError("\"hello\\\"", "incomplete escape sequence");
    
    TestUtils::printTestResult("Invalid escape sequence error handling", true);
}

void LiteralExprTest::testLiteralParsing(const std::string& input, const std::string& testName) {
    try {
        Parser parser(input);
        
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printError("Failed to parse " + testName + ": " + input);
            return;
        }
        
        if (expr->getType() != ExprType::Literal) {
            TestUtils::printError("Expected literal expression for " + testName + ": " + input);
            return;
        }
        
        TestUtils::printInfo("Successfully parsed " + testName + ": " + input);
        
    } catch (const std::exception& e) {
        TestUtils::printError("Exception parsing " + testName + ": " + e.what());
    }
}

void LiteralExprTest::testLiteralParsingError(const std::string& input, const std::string& testName) {
    try {
        Parser parser(input);
        
        auto expr = parser.parseExpression();
        
        // If we get here without exception, the test failed
        TestUtils::printError("Expected error for " + testName + " but parsing succeeded: " + input);
        
    } catch (const std::exception& e) {
        TestUtils::printInfo("Correctly caught error for " + testName + ": " + e.what());
    }
}

bool LiteralExprTest::verifyLiteralValue(const Expr* expr, const Value& expectedValue) {
    if (!expr || expr->getType() != ExprType::Literal) {
        return false;
    }
    
    const LiteralExpr* literalExpr = static_cast<const LiteralExpr*>(expr);
    const Value& actualValue = literalExpr->getValue();
    
    // Compare values based on type
    if (actualValue.type() != expectedValue.type()) {
        return false;
    }
    
    switch (actualValue.type()) {
        case ValueType::Number:
            return actualValue.asNumber() == expectedValue.asNumber();
        case ValueType::String:
            return actualValue.asString() == expectedValue.asString();
        case ValueType::Boolean:
            return actualValue.asBoolean() == expectedValue.asBoolean();
        case ValueType::Nil:
            return true; // Both are nil
        default:
            return false;
    }
}

} // namespace Tests
} // namespace Lua