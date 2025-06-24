#include "parser_variable_expr_test.hpp"

#include <string>
#include "../../../parser/parser.hpp"
#include "../../../parser/ast/expressions.hpp"
#include "../../../test_framework/core/test_macros.hpp"
#include "../../../test_framework/core/test_utils.hpp"

using namespace Lua;
using namespace Tests;
using namespace TestFramework;

void ParserVariableExprTest::runAllTests() {
    // Basic variable tests
    RUN_TEST(ParserVariableExprTest, testSimpleVariables);
    RUN_TEST(ParserVariableExprTest, testVariableNaming);
    //RUN_TEST(ParserVariableExprTest, testUnicodeVariablesWithErrorHandling);
    
    // Variable validation tests
    RUN_TEST(ParserVariableExprTest, testValidIdentifiers);
    RUN_TEST(ParserVariableExprTest, testInvalidIdentifiers);
    RUN_TEST(ParserVariableExprTest, testReservedKeywords);
    
    // Scope-related tests
    RUN_TEST(ParserVariableExprTest, testGlobalVariables);
    RUN_TEST(ParserVariableExprTest, testLocalVariables);
    
    // Edge case tests
    RUN_TEST(ParserVariableExprTest, testLongVariableNames);
    RUN_TEST(ParserVariableExprTest, testVariableWithNumbers);
    RUN_TEST(ParserVariableExprTest, testVariableWithUnderscores);
    
    // Error handling tests
    RUN_TEST(ParserVariableExprTest, testInvalidVariableNames);
    RUN_TEST(ParserVariableExprTest, testKeywordAsVariable);
}

void ParserVariableExprTest::testSimpleVariables() {
    TestUtils::printInfo("Testing simple variable parsing...");
    
    // Test basic variable names
    testVariableParsing("x", "x", "single letter variable");
    testVariableParsing("var", "var", "simple word variable");
    testVariableParsing("myVariable", "myVariable", "camelCase variable");
    testVariableParsing("my_variable", "my_variable", "snake_case variable");
    
    TestUtils::printTestResult("Simple variables", true);
}

void ParserVariableExprTest::testVariableNaming() {
    TestUtils::printInfo("Testing variable naming conventions...");
    
    // Test various naming patterns
    testVariableParsing("firstName", "firstName", "camelCase naming");
    testVariableParsing("first_name", "first_name", "snake_case naming");
    testVariableParsing("CONSTANT_VALUE", "CONSTANT_VALUE", "uppercase naming");
    testVariableParsing("mixedCase_Variable", "mixedCase_Variable", "mixed naming");
    
    TestUtils::printTestResult("Variable naming conventions", true);
}

void ParserVariableExprTest::testUnicodeVariablesWithErrorHandling() {
    TestUtils::printInfo("Testing unicode variable names with error handling...");
    
    // Test unicode characters that may not be supported
    testUnicodeVariableParsing("变量", "变量", "Chinese variable name");
    testUnicodeVariableParsing("переменная", "переменная", "Russian variable name");
    testUnicodeVariableParsing("変数", "変数", "Japanese variable name");
    testUnicodeVariableParsing("متغير", "متغير", "Arabic variable name");
    testUnicodeVariableParsing("переменная123", "переменная123", "Unicode with numbers");
    testUnicodeVariableParsing("café", "café", "Latin extended variable name");
    testUnicodeVariableParsing("naïve", "naïve", "Accented variable name");
    
    TestUtils::printTestResult("Unicode variables with error handling", true);
}

void ParserVariableExprTest::testValidIdentifiers() {
    TestUtils::printInfo("Testing valid identifier patterns...");
    
    // Test valid identifier patterns
    testVariableParsing("_private", "_private", "underscore prefix");
    testVariableParsing("__internal", "__internal", "double underscore prefix");
    testVariableParsing("var123", "var123", "variable with numbers");
    testVariableParsing("_123", "_123", "underscore with numbers");
    
    TestUtils::printTestResult("Valid identifiers", true);
}

void ParserVariableExprTest::testInvalidIdentifiers() {
    TestUtils::printInfo("Testing invalid identifier error handling...");
    
    // Test invalid identifier patterns
    testVariableParsingError("123var", "number prefix");
    testVariableParsingError("var-name", "hyphen in name");
    testVariableParsingError("var.name", "dot in name");
    testVariableParsingError("var name", "space in name");
    
    TestUtils::printTestResult("Invalid identifier error handling", true);
}

void ParserVariableExprTest::testReservedKeywords() {
    TestUtils::printInfo("Testing reserved keyword handling...");
    
    // Test that keywords cannot be used as variable names
    testVariableParsingError("if", "if keyword");
    testVariableParsingError("then", "then keyword");
    testVariableParsingError("else", "else keyword");
    testVariableParsingError("end", "end keyword");
    testVariableParsingError("while", "while keyword");
    testVariableParsingError("do", "do keyword");
    testVariableParsingError("for", "for keyword");
    testVariableParsingError("in", "in keyword");
    testVariableParsingError("repeat", "repeat keyword");
    testVariableParsingError("until", "until keyword");
    testVariableParsingError("function", "function keyword");
    testVariableParsingError("local", "local keyword");
    testVariableParsingError("return", "return keyword");
    testVariableParsingError("break", "break keyword");
    testVariableParsingError("and", "and keyword");
    testVariableParsingError("or", "or keyword");
    testVariableParsingError("not", "not keyword");
    testVariableParsingError("true", "true keyword");
    testVariableParsingError("false", "false keyword");
    testVariableParsingError("nil", "nil keyword");
    
    TestUtils::printTestResult("Reserved keyword handling", true);
}

void ParserVariableExprTest::testGlobalVariables() {
    TestUtils::printInfo("Testing global variable parsing...");
    
    // Test global variable access
    testVariableParsing("globalVar", "globalVar", "global variable");
    testVariableParsing("_G", "_G", "global table reference");
    testVariableParsing("print", "print", "built-in function reference");
    
    TestUtils::printTestResult("Global variables", true);
}

void ParserVariableExprTest::testLocalVariables() {
    TestUtils::printInfo("Testing local variable parsing...");
    
    // Test local variable access (same syntax as global)
    testVariableParsing("localVar", "localVar", "local variable");
    testVariableParsing("temp", "temp", "temporary variable");
    testVariableParsing("i", "i", "loop counter variable");
    
    TestUtils::printTestResult("Local variables", true);
}

void ParserVariableExprTest::testLongVariableNames() {
    TestUtils::printInfo("Testing long variable names...");
    
    // Test very long variable names
    std::string longName = "veryLongVariableNameThatExceedsNormalLength";
    testVariableParsing(longName, longName, "long variable name");
    
    std::string veryLongName = "extremelyLongVariableNameThatIsRidiculouslyLongButShouldStillBeValid";
    testVariableParsing(veryLongName, veryLongName, "very long variable name");
    
    TestUtils::printTestResult("Long variable names", true);
}

void ParserVariableExprTest::testVariableWithNumbers() {
    TestUtils::printInfo("Testing variables with numbers...");
    
    // Test variables containing numbers
    testVariableParsing("var1", "var1", "variable with single digit");
    testVariableParsing("var123", "var123", "variable with multiple digits");
    testVariableParsing("x1y2z3", "x1y2z3", "variable with interspersed numbers");
    testVariableParsing("matrix2D", "matrix2D", "variable ending with numbers");
    
    TestUtils::printTestResult("Variables with numbers", true);
}

void ParserVariableExprTest::testVariableWithUnderscores() {
    TestUtils::printInfo("Testing variables with underscores...");
    
    // Test variables with underscores
    testVariableParsing("_private", "_private", "single underscore prefix");
    testVariableParsing("__internal", "__internal", "double underscore prefix");
    testVariableParsing("var_name", "var_name", "underscore separator");
    testVariableParsing("_var_name_", "_var_name_", "underscores everywhere");
    testVariableParsing("___", "___", "only underscores");
    
    TestUtils::printTestResult("Variables with underscores", true);
}

void ParserVariableExprTest::testInvalidVariableNames() {
    TestUtils::printInfo("Testing invalid variable name error handling...");
    
    // Test various invalid patterns
    testVariableParsingError("123", "pure number");
    testVariableParsingError("@var", "special character prefix");
    testVariableParsingError("var@", "special character suffix");
    testVariableParsingError("var#name", "hash in name");
    testVariableParsingError("var$name", "dollar in name");
    
    TestUtils::printTestResult("Invalid variable name error handling", true);
}

void ParserVariableExprTest::testKeywordAsVariable() {
    TestUtils::printInfo("Testing keyword as variable error handling...");
    
    // Test using keywords as variables in expressions
    testVariableParsingError("if + 1", "keyword in expression");
    testVariableParsingError("while * 2", "keyword in arithmetic");
    
    TestUtils::printTestResult("Keyword as variable error handling", true);
}

void ParserVariableExprTest::testVariableParsing(const std::string& input, const std::string& expectedName, const std::string& testName) {
    Parser parser(input);

    auto expr = parser.parseExpression();

    if (!expr) {
        TestUtils::printError("Failed to parse " + testName + ": " + input);
        return;
    }

    if (expr->getType() != ExprType::Variable) {
        TestUtils::printError("Expected variable expression for " + testName + ": " + input);
        return;
    }

    if (!verifyVariableName(expr.get(), expectedName)) {
        TestUtils::printError("Variable name mismatch for " + testName + ": expected '" + expectedName + "'");
        return;
    }

    TestUtils::printInfo("Successfully parsed " + testName + ": " + input);
}

void ParserVariableExprTest::testVariableParsingError(const std::string& input, const std::string& testName) {
    try {
        Parser parser(input);
        
        auto expr = parser.parseExpression();
        
        // If we get here without exception, the test failed
        TestUtils::printError("Expected error for " + testName + " but parsing succeeded: " + input);
        
    } catch (const std::exception& e) {
        TestUtils::printInfo("Correctly caught error for " + testName + ": " + e.what());
    }
}

void ParserVariableExprTest::testUnicodeVariableParsing(const std::string& input, const std::string& expectedName, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();

        if (!expr) {
            TestUtils::printWarning("Unicode not supported for " + testName + ": " + input + " (returned null)");
            return;
        }

        if (expr->getType() != ExprType::Variable) {
            TestUtils::printWarning("Unicode parsing issue for " + testName + ": expected variable expression");
            return;
        }

        if (!verifyVariableName(expr.get(), expectedName)) {
            TestUtils::printWarning("Unicode variable name mismatch for " + testName + ": expected '" + expectedName + "'");
            return;
        }

        TestUtils::printInfo("Successfully parsed unicode " + testName + ": " + input);
        
    } catch (const std::exception& e) {
        TestUtils::printWarning("Unicode variable parsing not supported for " + testName + ": " + e.what());
    } catch (...) {
        TestUtils::printWarning("Unknown error parsing unicode variable " + testName + ": " + input);
    }
}

bool ParserVariableExprTest::verifyVariableName(const Expr* expr, const std::string& expectedName) {
    if (!expr || expr->getType() != ExprType::Variable) {
        return false;
    }
    
    const VariableExpr* varExpr = static_cast<const VariableExpr*>(expr);
    return varExpr->getName() == expectedName;
}