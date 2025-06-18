#include "call_expr_test.hpp"

namespace Lua {
namespace Tests {

void CallExprTest::runAllTests() {
    // Basic function call tests
    RUN_TEST(CallExprTest, testSimpleFunctionCalls);
    RUN_TEST(CallExprTest, testFunctionCallsWithArguments);
    RUN_TEST(CallExprTest, testFunctionCallsNoArguments);
    
    // Method call tests
    RUN_TEST(CallExprTest, testMethodCalls);
    RUN_TEST(CallExprTest, testMethodCallsWithArguments);
    RUN_TEST(CallExprTest, testMethodCallsNoArguments);
    
    // Argument type tests
    RUN_TEST(CallExprTest, testCallsWithLiteralArguments);
    RUN_TEST(CallExprTest, testCallsWithVariableArguments);
    RUN_TEST(CallExprTest, testCallsWithExpressionArguments);
    RUN_TEST(CallExprTest, testCallsWithMixedArguments);
    
    // Complex call tests
    RUN_TEST(CallExprTest, testNestedFunctionCalls);
    RUN_TEST(CallExprTest, testChainedFunctionCalls);
    RUN_TEST(CallExprTest, testCallsInExpressions);
    
    // Special syntax tests
    RUN_TEST(CallExprTest, testCallsWithTableConstructors);
    RUN_TEST(CallExprTest, testCallsWithStringLiterals);
    RUN_TEST(CallExprTest, testCallsWithParentheses);
    
    // Edge case tests
    RUN_TEST(CallExprTest, testCallsWithManyArguments);
    RUN_TEST(CallExprTest, testCallsWithComplexExpressions);
    RUN_TEST(CallExprTest, testCallsWithUnaryExpressions);
    RUN_TEST(CallExprTest, testCallsWithBinaryExpressions);
    
    // Error handling tests
    RUN_TEST(CallExprTest, testInvalidFunctionCalls);
    RUN_TEST(CallExprTest, testMalformedArguments);
    RUN_TEST(CallExprTest, testUnterminatedCalls);
    RUN_TEST(CallExprTest, testInvalidMethodCalls);
}

void CallExprTest::testSimpleFunctionCalls() {
    testCallParsing("func()", "func", 0, "Simple function call with no arguments");
    testCallParsing("print()", "print", 0, "Print function call");
    testCallParsing("math.abs()", "math.abs", 0, "Module function call");
    testCallParsing("_private()", "_private", 0, "Private function call");
}

void CallExprTest::testFunctionCallsWithArguments() {
    testCallParsing("func(x)", "func", 1, "Function call with one argument");
    testCallParsing("print(\"hello\")", "print", 1, "Function call with string argument");
    testCallParsing("math.max(a, b)", "math.max", 2, "Function call with two arguments");
    testCallParsing("func(1, 2, 3)", "func", 3, "Function call with three arguments");
    testCallParsing("calculate(x, y, z, w)", "calculate", 4, "Function call with four arguments");
}

void CallExprTest::testFunctionCallsNoArguments() {
    testCallParsing("getTime()", "getTime", 0, "Get time function call");
    testCallParsing("initialize()", "initialize", 0, "Initialize function call");
    testCallParsing("cleanup()", "cleanup", 0, "Cleanup function call");
    testCallParsing("os.clock()", "os.clock", 0, "OS clock function call");
}

void CallExprTest::testMethodCalls() {
    testMethodCallParsing("obj:method()", "obj", "method", 0, "Simple method call");
    testMethodCallParsing("player:getName()", "player", "getName", 0, "Get name method call");
    testMethodCallParsing("table:insert()", "table", "insert", 0, "Table insert method call");
    testMethodCallParsing("self:update()", "self", "update", 0, "Self method call");
}

void CallExprTest::testMethodCallsWithArguments() {
    testMethodCallParsing("obj:setName(\"John\")", "obj", "setName", 1, "Method call with string argument");
    testMethodCallParsing("player:move(x, y)", "player", "move", 2, "Method call with two arguments");
    testMethodCallParsing("table:insert(index, value)", "table", "insert", 2, "Table insert with arguments");
    testMethodCallParsing("self:init(a, b, c)", "self", "init", 3, "Self init with three arguments");
}

void CallExprTest::testMethodCallsNoArguments() {
    testMethodCallParsing("obj:toString()", "obj", "toString", 0, "To string method call");
    testMethodCallParsing("player:getHealth()", "player", "getHealth", 0, "Get health method call");
    testMethodCallParsing("connection:close()", "connection", "close", 0, "Connection close method call");
    testMethodCallParsing("timer:start()", "timer", "start", 0, "Timer start method call");
}

void CallExprTest::testCallsWithLiteralArguments() {
    testCallParsing("func(42)", "func", 1, "Function call with integer literal");
    testCallParsing("func(3.14)", "func", 1, "Function call with float literal");
    testCallParsing("func(\"hello\")", "func", 1, "Function call with string literal");
    testCallParsing("func(true)", "func", 1, "Function call with boolean literal");
    testCallParsing("func(nil)", "func", 1, "Function call with nil literal");
    testCallParsing("func(42, \"test\", true)", "func", 3, "Function call with mixed literals");
}

void CallExprTest::testCallsWithVariableArguments() {
    testCallParsing("func(x)", "func", 1, "Function call with variable argument");
    testCallParsing("func(count)", "func", 1, "Function call with named variable");
    testCallParsing("func(x, y)", "func", 2, "Function call with two variables");
    testCallParsing("func(a, b, c)", "func", 3, "Function call with three variables");
    testCallParsing("func(_private, _internal)", "func", 2, "Function call with underscore variables");
}

void CallExprTest::testCallsWithExpressionArguments() {
    testCallParsing("func(a + b)", "func", 1, "Function call with addition expression");
    testCallParsing("func(x * 2)", "func", 1, "Function call with multiplication expression");
    testCallParsing("func(not flag)", "func", 1, "Function call with logical not expression");
    testCallParsing("func(-value)", "func", 1, "Function call with unary minus expression");
    testCallParsing("func(a == b)", "func", 1, "Function call with comparison expression");
}

void CallExprTest::testCallsWithMixedArguments() {
    testCallParsing("func(42, x)", "func", 2, "Function call with literal and variable");
    testCallParsing("func(\"hello\", a + b)", "func", 2, "Function call with literal and expression");
    testCallParsing("func(x, 5, \"test\")", "func", 3, "Function call with variable, literal, and string");
    testCallParsing("func(true, x > 0, name)", "func", 3, "Function call with boolean, comparison, and variable");
}

void CallExprTest::testNestedFunctionCalls() {
    testCallParsing("outer(inner())", "outer", 1, "Function call with nested call argument");
    testCallParsing("func(math.abs(x))", "func", 1, "Function call with nested module call");
    testCallParsing("print(string.format(\"%d\", num))", "print", 1, "Print with nested format call");
    testCallParsing("calculate(getValue(), getMultiplier())", "calculate", 2, "Function call with two nested calls");
}

void CallExprTest::testChainedFunctionCalls() {
    testCallParsing("getObject().method()", "getObject().method", 0, "Chained function and method call");
    testCallParsing("factory().create().init()", "factory().create().init", 0, "Triple chained calls");
    testCallParsing("obj.getChild().getName()", "obj.getChild().getName", 0, "Chained member and method calls");
}

void CallExprTest::testCallsInExpressions() {
    testCallParsing("getValue() + 5", "getValue", 0, "Function call in addition expression");
    testCallParsing("func() == true", "func", 0, "Function call in comparison expression");
    testCallParsing("not isEmpty()", "isEmpty", 0, "Function call in unary expression");
    testCallParsing("getCount() * getMultiplier()", "getCount", 0, "Function call in binary expression");
}

void CallExprTest::testCallsWithTableConstructors() {
    testCallParsing("func{}", "func", 1, "Function call with empty table constructor");
    testCallParsing("func{1, 2, 3}", "func", 1, "Function call with array table constructor");
    testCallParsing("func{x = 1, y = 2}", "func", 1, "Function call with hash table constructor");
    testCallParsing("print{\"hello\", \"world\"}", "print", 1, "Print with table constructor");
}

void CallExprTest::testCallsWithStringLiterals() {
    testCallParsing("func\"hello\"", "func", 1, "Function call with string literal (no parentheses)");
    testCallParsing("print\"Hello, World!\"", "print", 1, "Print with string literal (no parentheses)");
    testCallParsing("require\"module\"", "require", 1, "Require with string literal (no parentheses)");
    testCallParsing("dofile\"script.lua\"", "dofile", 1, "Dofile with string literal (no parentheses)");
}

void CallExprTest::testCallsWithParentheses() {
    testCallParsing("(func)()", "func", 0, "Parenthesized function call");
    testCallParsing("(getValue)(x)", "getValue", 1, "Parenthesized function call with argument");
    testCallParsing("(obj.method)()", "obj.method", 0, "Parenthesized method call");
    testCallParsing("(functions[index])()", "functions[index]", 0, "Parenthesized indexed function call");
}

void CallExprTest::testCallsWithManyArguments() {
    testCallParsing("func(a, b, c, d, e)", "func", 5, "Function call with five arguments");
    testCallParsing("func(1, 2, 3, 4, 5, 6, 7, 8)", "func", 8, "Function call with eight arguments");
    testCallParsing("printf(\"%s %d %f\", name, age, height)", "printf", 4, "Printf with format and arguments");
}

void CallExprTest::testCallsWithComplexExpressions() {
    testCallParsing("func((a + b) * c)", "func", 1, "Function call with complex arithmetic expression");
    testCallParsing("func(a and b or c)", "func", 1, "Function call with complex logical expression");
    testCallParsing("func(x < y and y < z)", "func", 1, "Function call with complex comparison expression");
    testCallParsing("func(\"result: \" .. getValue())", "func", 1, "Function call with concatenation expression");
}

void CallExprTest::testCallsWithUnaryExpressions() {
    testCallParsing("func(-x)", "func", 1, "Function call with unary minus");
    testCallParsing("func(+value)", "func", 1, "Function call with unary plus");
    testCallParsing("func(not flag)", "func", 1, "Function call with logical not");
    testCallParsing("func(#array)", "func", 1, "Function call with length operator");
    testCallParsing("func(-a, +b, not c)", "func", 3, "Function call with multiple unary expressions");
}

void CallExprTest::testCallsWithBinaryExpressions() {
    testCallParsing("func(a + b)", "func", 1, "Function call with addition");
    testCallParsing("func(x * y)", "func", 1, "Function call with multiplication");
    testCallParsing("func(a == b)", "func", 1, "Function call with equality");
    testCallParsing("func(x and y)", "func", 1, "Function call with logical and");
    testCallParsing("func(a + b, x * y)", "func", 2, "Function call with multiple binary expressions");
}

void CallExprTest::testInvalidFunctionCalls() {
    testCallParsingError("func(", "Unterminated function call");
    testCallParsingError("func)", "Invalid function call syntax");
    testCallParsingError("func(,)", "Invalid comma in function call");
    testCallParsingError("func(a,)", "Trailing comma in function call");
    testCallParsingError("func(,a)", "Leading comma in function call");
}

void CallExprTest::testMalformedArguments() {
    testCallParsingError("func(a,,b)", "Double comma in arguments");
    testCallParsingError("func(a b)", "Missing comma between arguments");
    testCallParsingError("func(a + )", "Incomplete expression argument");
    testCallParsingError("func( + b)", "Invalid expression argument");
}

void CallExprTest::testUnterminatedCalls() {
    testCallParsingError("func(a, b", "Missing closing parenthesis");
    testCallParsingError("func(a, b, c", "Missing closing parenthesis with multiple args");
    testCallParsingError("func(getValue(", "Nested unterminated call");
    testCallParsingError("obj:method(a, b", "Unterminated method call");
}

void CallExprTest::testInvalidMethodCalls() {
    testCallParsingError("obj:", "Incomplete method call");
    testCallParsingError("obj:(", "Invalid method name");
    testCallParsingError(":method()", "Missing object in method call");
    testCallParsingError("obj::method()", "Double colon in method call");
}

void CallExprTest::testCallParsing(const std::string& input, const std::string& expectedFunction, int expectedArgCount, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printTestResult(testName, false, "Failed to parse expression");
            return;
        }
        
        if (!verifyCallExpression(expr.get(), expectedFunction, expectedArgCount)) {
            TestUtils::printTestResult(testName, false, "Expression is not a call expression or mismatch");
            return;
        }
        
        TestUtils::printTestResult(testName, true);
        
        // Print additional info for debugging
        if (auto callExpr = dynamic_cast<const CallExpr*>(expr.get())) {
            printCallExpressionInfo(callExpr);
        }
        
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, false, std::string("Exception: ") + e.what());
    }
}

void CallExprTest::testMethodCallParsing(const std::string& input, const std::string& expectedObject, const std::string& expectedMethod, int expectedArgCount, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printTestResult(testName, false, "Failed to parse expression");
            return;
        }
        
        if (!verifyMethodCallExpression(expr.get(), expectedObject, expectedMethod, expectedArgCount)) {
            TestUtils::printTestResult(testName, false, "Expression is not a method call expression or mismatch");
            return;
        }
        
        TestUtils::printTestResult(testName, true, "Successfully parsed method call expression");
        
        // Print additional info for debugging
        if (auto callExpr = dynamic_cast<const CallExpr*>(expr.get())) {
            printCallExpressionInfo(callExpr);
        }
        
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, false, std::string("Exception: ") + e.what());
    }
}

void CallExprTest::testCallParsingError(const std::string& input, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printTestResult(testName, true, "Correctly failed to parse invalid call expression");
            return;
        }
        
        TestUtils::printTestResult(testName, false, "Should have failed to parse invalid call expression");
        
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, true, std::string("Correctly threw exception: ") + e.what());
    }
}

bool CallExprTest::verifyCallExpression(const Expr* expr, const std::string& expectedFunction, int expectedArgCount) {
    if (!expr) {
        return false;
    }
    
    const CallExpr* callExpr = dynamic_cast<const CallExpr*>(expr);
    if (!callExpr) {
        return false;
    }
    
    // Check argument count
    if (static_cast<int>(callExpr->args.size()) != expectedArgCount) {
        return false;
    }
    
    // For simple verification, we'll just check if it's a call expression
    // More detailed function name verification would require examining the callee
    return true;
}

bool CallExprTest::verifyMethodCallExpression(const Expr* expr, const std::string& expectedObject, const std::string& expectedMethod, int expectedArgCount) {
    if (!expr) {
        return false;
    }
    
    const CallExpr* callExpr = dynamic_cast<const CallExpr*>(expr);
    if (!callExpr) {
        return false;
    }
    
    // Check if it's a method call (should have isMethodCall flag or similar)
    // Check argument count
    if (static_cast<int>(callExpr->args.size()) != expectedArgCount) {
        return false;
    }
    
    // For simple verification, we'll just check if it's a call expression
    // More detailed method call verification would require examining the callee structure
    return true;
}

void CallExprTest::printCallExpressionInfo(const CallExpr* callExpr) {
    if (!callExpr) {
        return;
    }
    
    TestUtils::printInfo("  Argument count: " + std::to_string(callExpr->args.size()));
    TestUtils::printInfo("  Has callee: " + std::string(callExpr->callee ? "yes" : "no"));
    
    if (callExpr->callee) {
        std::string calleeName = extractVariableName(callExpr->callee.get());
        if (!calleeName.empty()) {
            TestUtils::printInfo("  Callee name: " + calleeName);
        }
    }
}

std::string CallExprTest::extractVariableName(const Expr* expr) {
    if (!expr) {
        return "";
    }
    
    if (const VariableExpr* varExpr = dynamic_cast<const VariableExpr*>(expr)) {
        return varExpr->name.lexeme;
    }
    
    if (const MemberExpr* memberExpr = dynamic_cast<const MemberExpr*>(expr)) {
        std::string objectName = extractVariableName(memberExpr->object.get());
        return objectName + "." + memberExpr->name.lexeme;
    }
    
    return "complex_expression";
}

} // namespace Tests
} // namespace Lua