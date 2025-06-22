#ifndef CALL_EXPR_TEST_HPP
#define CALL_EXPR_TEST_HPP

#include <iostream>
#include "../../../parser/parser.hpp"
#include "../../../parser/ast/expressions.hpp"
#include "../../../test_framework/core/test_macros.hpp"

namespace Lua {
namespace Tests {
namespace Parser {
namespace Expr {

/**
 * @brief Call Expression Parser Test Class
 * 
 * Namespace: Lua::Tests::Parser::Expr
 * 
 * Tests parsing of function call expressions including:
 * - Simple function calls
 * - Method calls with colon syntax
 * - Calls with various argument types
 * - Nested function calls
 * - Error handling for malformed calls
 */
class CallExprTest {
public:
    /**
     * @brief Run all call expression tests
     * 
     * Executes all test cases for call expression parsing
     */
    static void runAllTests();
    
private:
    // Basic function call tests
    static void testSimpleFunctionCalls();
    static void testFunctionCallsWithArguments();
    static void testFunctionCallsNoArguments();
    
    // Method call tests
    static void testMethodCalls();
    static void testMethodCallsWithArguments();
    static void testMethodCallsNoArguments();
    
    // Argument type tests
    static void testCallsWithLiteralArguments();
    static void testCallsWithVariableArguments();
    static void testCallsWithExpressionArguments();
    static void testCallsWithMixedArguments();
    
    // Complex call tests
    static void testNestedFunctionCalls();
    static void testChainedFunctionCalls();
    static void testCallsInExpressions();
    
    // Special syntax tests
    static void testCallsWithTableConstructors();
    static void testCallsWithStringLiterals();
    static void testCallsWithParentheses();
    
    // Edge case tests
    static void testCallsWithManyArguments();
    static void testCallsWithComplexExpressions();
    static void testCallsWithUnaryExpressions();
    static void testCallsWithBinaryExpressions();
    
    // Error handling tests
    static void testInvalidFunctionCalls();
    static void testMalformedArguments();
    static void testUnterminatedCalls();
    static void testInvalidMethodCalls();
    
    // Helper methods
    static void testCallParsing(const std::string& input, const std::string& expectedFunction, int expectedArgCount, const std::string& testName);
    static void testMethodCallParsing(const std::string& input, const std::string& expectedObject, const std::string& expectedMethod, int expectedArgCount, const std::string& testName);
    static void testCallParsingError(const std::string& input, const std::string& testName);
    //static bool verifyCallExpression(const Expr* expr, const std::string& expectedFunction, int expectedArgCount);
    //static bool verifyMethodCallExpression(const Expr* expr, const std::string& expectedObject, const std::string& expectedMethod, int expectedArgCount);
    static void printCallExpressionInfo(const CallExpr* callExpr);
    //static std::string extractVariableName(const Expr* expr);
};

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua

#endif // CALL_EXPR_TEST_HPP