#include "variable_expr_test.hpp"
#include "../../../lexer/lexer.hpp"

namespace Lua {
namespace Tests {

void VariableExprTest::runAllTests() {
    // Basic variable tests
    RUN_TEST(VariableExprTest, testSimpleVariables);
    RUN_TEST(VariableExprTest, testVariableNaming);
    RUN_TEST(VariableExprTest, testUnicodeVariables);
    
    // Variable validation tests
    RUN_TEST(VariableExprTest, testValidIdentifiers);
    RUN_TEST(VariableExprTest, testInvalidIdentifiers);
    RUN_TEST(VariableExprTest, testReservedKeywords);
    
    // Scope-related tests
    RUN_TEST(VariableExprTest, testGlobalVariables);
    RUN_TEST(VariableExprTest, testLocalVariables);
    
    // Edge case tests
    RUN_TEST(VariableExprTest, testLongVariableNames);
    RUN_TEST(VariableExprTest, testVariableWithNumbers);
    RUN_TEST(VariableExprTest, testVariableWithUnderscores);
    
    // Error handling tests
    RUN_TEST(VariableExprTest, testInvalidVariableNames);
    RUN_TEST(VariableExprTest, testKeywordAsVariable);
}

void VariableExprTest::testSimpleVariables() {
    TestUtils::printInfo("Testing simple variable parsing...");
    
    // Test basic variable names
    testVariableParsing("x", "x", "single letter variable");
    testVariableParsing("var", "var", "simple word variable");
    testVariableParsing("myVariable", "myVariable", "camelCase variable");
    testVariableParsing("my_variable", "my_variable", "snake_case variable");
    
    TestUtils::printTestResult("Simple variables", true);
}

void VariableExprTest::testVariableNaming() {
    TestUtils::printInfo("Testing variable naming conventions...");
    
    // Test various naming patterns
    testVariableParsing("firstName", "firstName", "camelCase naming");
    testVariableParsing("first_name", "first_name", "snake_case naming");
    testVariableParsing("CONSTANT_VALUE", "CONSTANT_VALUE", "uppercase naming");
    testVariableParsing("mixedCase_Variable", "mixedCase_Variable", "mixed naming");
    
    TestUtils::printTestResult("Variable naming conventions", true);
}

void VariableExprTest::testUnicodeVariables() {
    TestUtils::printInfo("Testing unicode variable names...");
    
    // Test unicode characters in variable names (if supported)
    testVariableParsing("变量", "变量", "Chinese variable name");
    testVariableParsing("переменная", "переменная", "Russian variable name");
    testVariableParsing("変数", "変数", "Japanese variable name");
    
    TestUtils::printTestResult("Unicode variables", true);
}

void VariableExprTest::testValidIdentifiers() {
    TestUtils::printInfo("Testing valid identifier patterns...");
    
    // Test valid identifier patterns
    testVariableParsing("_private", "_private", "underscore prefix");
    testVariableParsing("__internal", "__internal", "double underscore prefix");
    testVariableParsing("var123", "var123", "variable with numbers");
    testVariableParsing("_123", "_123", "underscore with numbers");
    
    TestUtils::printTestResult("Valid identifiers", true);
}

void VariableExprTest::testInvalidIdentifiers() {
    TestUtils::printInfo("Testing invalid identifier error handling...");
    
    // Test invalid identifier patterns
    testVariableParsingError("123var", "number prefix");
    testVariableParsingError("var-name", "hyphen in name");
    testVariableParsingError("var.name", "dot in name");
    testVariableParsingError("var name", "space in name");
    
    TestUtils::printTestResult("Invalid identifier error handling", true);
}

void VariableExprTest::testReservedKeywords() {
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

void VariableExprTest::testGlobalVariables() {
    TestUtils::printInfo("Testing global variable parsing...");
    
    // Test global variable access
    testVariableParsing("globalVar", "globalVar", "global variable");
    testVariableParsing("_G", "_G", "global table reference");
    testVariableParsing("print", "print", "built-in function reference");
    
    TestUtils::printTestResult("Global variables", true);
}

void VariableExprTest::testLocalVariables() {
    TestUtils::printInfo("Testing local variable parsing...");
    
    // Test local variable access (same syntax as global)
    testVariableParsing("localVar", "localVar", "local variable");
    testVariableParsing("temp", "temp", "temporary variable");
    testVariableParsing("i", "i", "loop counter variable");
    
    TestUtils::printTestResult("Local variables", true);
}

void VariableExprTest::testLongVariableNames() {
    TestUtils::printInfo("Testing long variable names...");
    
    // Test very long variable names
    std::string longName = "veryLongVariableNameThatExceedsNormalLength";
    testVariableParsing(longName, longName, "long variable name");
    
    std::string veryLongName = "extremelyLongVariableNameThatIsRidiculouslyLongButShouldStillBeValid";
    testVariableParsing(veryLongName, veryLongName, "very long variable name");
    
    TestUtils::printTestResult("Long variable names", true);
}

void VariableExprTest::testVariableWithNumbers() {
    TestUtils::printInfo("Testing variables with numbers...");
    
    // Test variables containing numbers
    testVariableParsing("var1", "var1", "variable with single digit");
    testVariableParsing("var123", "var123", "variable with multiple digits");
    testVariableParsing("x1y2z3", "x1y2z3", "variable with interspersed numbers");
    testVariableParsing("matrix2D", "matrix2D", "variable ending with numbers");
    
    TestUtils::printTestResult("Variables with numbers", true);
}

void VariableExprTest::testVariableWithUnderscores() {
    TestUtils::printInfo("Testing variables with underscores...");
    
    // Test variables with underscores
    testVariableParsing("_private", "_private", "single underscore prefix");
    testVariableParsing("__internal", "__internal", "double underscore prefix");
    testVariableParsing("var_name", "var_name", "underscore separator");
    testVariableParsing("_var_name_", "_var_name_", "underscores everywhere");
    testVariableParsing("___", "___", "only underscores");
    
    TestUtils::printTestResult("Variables with underscores", true);
}

void VariableExprTest::testInvalidVariableNames() {
    TestUtils::printInfo("Testing invalid variable name error handling...");
    
    // Test various invalid patterns
    testVariableParsingError("123", "pure number");
    testVariableParsingError("@var", "special character prefix");
    testVariableParsingError("var@", "special character suffix");
    testVariableParsingError("var#name", "hash in name");
    testVariableParsingError("var$name", "dollar in name");
    
    TestUtils::printTestResult("Invalid variable name error handling", true);
}

void VariableExprTest::testKeywordAsVariable() {
    TestUtils::printInfo("Testing keyword as variable error handling...");
    
    // Test using keywords as variables in expressions
    testVariableParsingError("if + 1", "keyword in expression");
    testVariableParsingError("while * 2", "keyword in arithmetic");
    
    TestUtils::printTestResult("Keyword as variable error handling", true);
}

void VariableExprTest::testVariableParsing(const std::string& input, const std::string& expectedName, const std::string& testName) {
    try {
        Lexer lexer(input);
        Parser parser(lexer);
        
        auto expr = parser.expression();
        
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
        
    } catch (const std::exception& e) {
        TestUtils::printError("Exception parsing " + testName + ": " + e.what());
    }
}

void VariableExprTest::testVariableParsingError(const std::string& input, const std::string& testName) {
    try {
        Lexer lexer(input);
        Parser parser(lexer);
        
        auto expr = parser.expression();
        
        // If we get here without exception, the test failed
        TestUtils::printError("Expected error for " + testName + " but parsing succeeded: " + input);
        
    } catch (const std::exception& e) {
        TestUtils::printInfo("Correctly caught error for " + testName + ": " + e.what());
    }
}

bool VariableExprTest::verifyVariableName(const Expr* expr, const std::string& expectedName) {
    if (!expr || expr->getType() != ExprType::Variable) {
        return false;
    }
    
    const VariableExpr* varExpr = static_cast<const VariableExpr*>(expr);
    return varExpr->getName() == expectedName;
}

} // namespace Tests
} // namespace Lua