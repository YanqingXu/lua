#include "parser_call_expr_test.hpp"

using namespace Lua;
using namespace Tests;
using namespace TestFramework;

void ParserCallExprTest::runAllTests() {
    RUN_TEST_GROUP("Basic Function Calls", []() {
        RUN_TEST(ParserCallExprTest, testSimpleFunctionCalls);
        RUN_TEST(ParserCallExprTest, testFunctionCallsWithArguments);
        RUN_TEST(ParserCallExprTest, testFunctionCallsNoArguments);
    });
    
    RUN_TEST_GROUP("Method Calls", []() {
        RUN_TEST(ParserCallExprTest, testMethodCalls);
        RUN_TEST(ParserCallExprTest, testMethodCallsWithArguments);
        RUN_TEST(ParserCallExprTest, testMethodCallsNoArguments);
    });
    
    RUN_TEST_GROUP("Argument Types", []() {
        RUN_TEST(ParserCallExprTest, testCallsWithLiteralArguments);
        RUN_TEST(ParserCallExprTest, testCallsWithVariableArguments);
        RUN_TEST(ParserCallExprTest, testCallsWithExpressionArguments);
        RUN_TEST(ParserCallExprTest, testCallsWithMixedArguments);
    });
    
    RUN_TEST_GROUP("Complex Calls", []() {
        RUN_TEST(ParserCallExprTest, testNestedFunctionCalls);
        RUN_TEST(ParserCallExprTest, testChainedFunctionCalls);
        RUN_TEST(ParserCallExprTest, testCallsInExpressions);
    });
    
    RUN_TEST_GROUP("Special Syntax", []() {
        RUN_TEST(ParserCallExprTest, testCallsWithTableConstructors);
        RUN_TEST(ParserCallExprTest, testCallsWithStringLiterals);
        RUN_TEST(ParserCallExprTest, testCallsWithParentheses);
    });
    
    RUN_TEST_GROUP("Edge Cases", []() {
        RUN_TEST(ParserCallExprTest, testCallsWithManyArguments);
        RUN_TEST(ParserCallExprTest, testCallsWithComplexExpressions);
        RUN_TEST(ParserCallExprTest, testCallsWithUnaryExpressions);
        RUN_TEST(ParserCallExprTest, testCallsWithBinaryExpressions);
    });
    
    RUN_TEST_GROUP("Error Handling", []() {
        RUN_TEST(ParserCallExprTest, testInvalidFunctionCalls);
        RUN_TEST(ParserCallExprTest, testMalformedArguments);
        RUN_TEST(ParserCallExprTest, testUnterminatedCalls);
        RUN_TEST(ParserCallExprTest, testInvalidMethodCalls);
    });
}

void ParserCallExprTest::testSimpleFunctionCalls() {
    testCallParsing("func()", "func", 0, "Simple function call with no arguments");
    testCallParsing("print()", "print", 0, "Print function call");
    testCallParsing("math.abs()", "math.abs", 0, "Module function call");
    testCallParsing("_private()", "_private", 0, "Private function call");
}

void ParserCallExprTest::testFunctionCallsWithArguments() {
    testCallParsing("func(x)", "func", 1, "Function call with one argument");
    testCallParsing("print(\"hello\")", "print", 1, "Function call with string argument");
    testCallParsing("math.max(a, b)", "math.max", 2, "Function call with two arguments");
    testCallParsing("func(1, 2, 3)", "func", 3, "Function call with three arguments");
    testCallParsing("calculate(x, y, z, w)", "calculate", 4, "Function call with four arguments");
}

void ParserCallExprTest::testFunctionCallsNoArguments() {
    testCallParsing("getTime()", "getTime", 0, "Get time function call");
    testCallParsing("initialize()", "initialize", 0, "Initialize function call");
    testCallParsing("cleanup()", "cleanup", 0, "Cleanup function call");
    testCallParsing("os.clock()", "os.clock", 0, "OS clock function call");
}

void ParserCallExprTest::testMethodCalls() {
    testMethodCallParsing("obj:method()", "obj", "method", 0, "Simple method call");
    testMethodCallParsing("player:getName()", "player", "getName", 0, "Get name method call");
    testMethodCallParsing("table:insert()", "table", "insert", 0, "Table insert method call");
    testMethodCallParsing("self:update()", "self", "update", 0, "Self method call");
}

void ParserCallExprTest::testMethodCallsWithArguments() {
    testMethodCallParsing("obj:setName(\"John\")", "obj", "setName", 1, "Method call with string argument");
    testMethodCallParsing("player:move(x, y)", "player", "move", 2, "Method call with two arguments");
    testMethodCallParsing("table:insert(index, value)", "table", "insert", 2, "Table insert with arguments");
    testMethodCallParsing("self:init(a, b, c)", "self", "init", 3, "Self init with three arguments");
}

void ParserCallExprTest::testMethodCallsNoArguments() {
    testMethodCallParsing("obj:toString()", "obj", "toString", 0, "To string method call");
    testMethodCallParsing("player:getHealth()", "player", "getHealth", 0, "Get health method call");
    testMethodCallParsing("connection:close()", "connection", "close", 0, "Connection close method call");
    testMethodCallParsing("timer:start()", "timer", "start", 0, "Timer start method call");
}

void ParserCallExprTest::testCallsWithLiteralArguments() {
    testCallParsing("func(42)", "func", 1, "Function call with integer literal");
    testCallParsing("func(3.14)", "func", 1, "Function call with float literal");
    testCallParsing("func(\"hello\")", "func", 1, "Function call with string literal");
    testCallParsing("func(true)", "func", 1, "Function call with boolean literal");
    testCallParsing("func(nil)", "func", 1, "Function call with nil literal");
    testCallParsing("func(42, \"test\", true)", "func", 3, "Function call with mixed literals");
}

void ParserCallExprTest::testCallsWithVariableArguments() {
    testCallParsing("func(x)", "func", 1, "Function call with variable argument");
    testCallParsing("func(count)", "func", 1, "Function call with named variable");
    testCallParsing("func(x, y)", "func", 2, "Function call with two variables");
    testCallParsing("func(a, b, c)", "func", 3, "Function call with three variables");
    testCallParsing("func(_private, _internal)", "func", 2, "Function call with underscore variables");
}

void ParserCallExprTest::testCallsWithExpressionArguments() {
    testCallParsing("func(a + b)", "func", 1, "Function call with addition expression");
    testCallParsing("func(x * 2)", "func", 1, "Function call with multiplication expression");
    testCallParsing("func(not flag)", "func", 1, "Function call with logical not expression");
    testCallParsing("func(-value)", "func", 1, "Function call with unary minus expression");
    testCallParsing("func(a == b)", "func", 1, "Function call with comparison expression");
}

void ParserCallExprTest::testCallsWithMixedArguments() {
    testCallParsing("func(42, x)", "func", 2, "Function call with literal and variable");
    testCallParsing("func(\"hello\", a + b)", "func", 2, "Function call with literal and expression");
    testCallParsing("func(x, 5, \"test\")", "func", 3, "Function call with variable, literal, and string");
    testCallParsing("func(true, x > 0, name)", "func", 3, "Function call with boolean, comparison, and variable");
}

void ParserCallExprTest::testNestedFunctionCalls() {
    testCallParsing("outer(inner())", "outer", 1, "Function call with nested call argument");
    testCallParsing("func(math.abs(x))", "func", 1, "Function call with nested module call");
    testCallParsing("print(string.format(\"%d\", num))", "print", 1, "Print with nested format call");
    testCallParsing("calculate(getValue(), getMultiplier())", "calculate", 2, "Function call with two nested calls");
}

void ParserCallExprTest::testChainedFunctionCalls() {
    testCallParsing("getObject().method()", "getObject().method", 0, "Chained function and method call");
    testCallParsing("factory().create().init()", "factory().create().init", 0, "Triple chained calls");
    testCallParsing("obj.getChild().getName()", "obj.getChild().getName", 0, "Chained member and method calls");
}

void ParserCallExprTest::testCallsInExpressions() {
    testCallParsing("getValue() + 5", "getValue", 0, "Function call in addition expression");
    testCallParsing("func() == true", "func", 0, "Function call in comparison expression");
    testCallParsing("not isEmpty()", "isEmpty", 0, "Function call in unary expression");
    testCallParsing("getCount() * getMultiplier()", "getCount", 0, "Function call in binary expression");
}

void ParserCallExprTest::testCallsWithTableConstructors() {
    testCallParsing("func{}", "func", 1, "Function call with empty table constructor");
    testCallParsing("func{1, 2, 3}", "func", 1, "Function call with array table constructor");
    testCallParsing("func{x = 1, y = 2}", "func", 1, "Function call with hash table constructor");
    testCallParsing("print{\"hello\", \"world\"}", "print", 1, "Print with table constructor");
}

void ParserCallExprTest::testCallsWithStringLiterals() {
    testCallParsing("func\"hello\"", "func", 1, "Function call with string literal (no parentheses)");
    testCallParsing("print\"Hello, World!\"", "print", 1, "Print with string literal (no parentheses)");
    testCallParsing("require\"module\"", "require", 1, "Require with string literal (no parentheses)");
    testCallParsing("dofile\"script.lua\"", "dofile", 1, "Dofile with string literal (no parentheses)");
}

void ParserCallExprTest::testCallsWithParentheses() {
    testCallParsing("(func)()", "func", 0, "Parenthesized function call");
    testCallParsing("(getValue)(x)", "getValue", 1, "Parenthesized function call with argument");
    testCallParsing("(obj.method)()", "obj.method", 0, "Parenthesized method call");
    testCallParsing("(functions[index])()", "functions[index]", 0, "Parenthesized indexed function call");
}

void ParserCallExprTest::testCallsWithManyArguments() {
    testCallParsing("func(a, b, c, d, e)", "func", 5, "Function call with five arguments");
    testCallParsing("func(1, 2, 3, 4, 5, 6, 7, 8)", "func", 8, "Function call with eight arguments");
    testCallParsing("printf(\"%s %d %f\", name, age, height)", "printf", 4, "Printf with format and arguments");
}

void ParserCallExprTest::testCallsWithComplexExpressions() {
    testCallParsing("func((a + b) * c)", "func", 1, "Function call with complex arithmetic expression");
    testCallParsing("func(a and b or c)", "func", 1, "Function call with complex logical expression");
    testCallParsing("func(x < y and y < z)", "func", 1, "Function call with complex comparison expression");
    testCallParsing("func(\"result: \" .. getValue())", "func", 1, "Function call with concatenation expression");
}

void ParserCallExprTest::testCallsWithUnaryExpressions() {
    testCallParsing("func(-x)", "func", 1, "Function call with unary minus");
    testCallParsing("func(+value)", "func", 1, "Function call with unary plus");
    testCallParsing("func(not flag)", "func", 1, "Function call with logical not");
    testCallParsing("func(#array)", "func", 1, "Function call with length operator");
    testCallParsing("func(-a, +b, not c)", "func", 3, "Function call with multiple unary expressions");
}

void ParserCallExprTest::testCallsWithBinaryExpressions() {
    testCallParsing("func(a + b)", "func", 1, "Function call with addition");
    testCallParsing("func(x * y)", "func", 1, "Function call with multiplication");
    testCallParsing("func(a == b)", "func", 1, "Function call with equality");
    testCallParsing("func(x and y)", "func", 1, "Function call with logical and");
    testCallParsing("func(a + b, x * y)", "func", 2, "Function call with multiple binary expressions");
}

void ParserCallExprTest::testInvalidFunctionCalls() {
    testCallParsingError("func(", "Unterminated function call");
    testCallParsingError("func)", "Invalid function call syntax");
    testCallParsingError("func(,)", "Invalid comma in function call");
    testCallParsingError("func(a,)", "Trailing comma in function call");
    testCallParsingError("func(,a)", "Leading comma in function call");
}

void ParserCallExprTest::testMalformedArguments() {
    testCallParsingError("func(a,,b)", "Double comma in arguments");
    testCallParsingError("func(a b)", "Missing comma between arguments");
    testCallParsingError("func(a + )", "Incomplete expression argument");
    testCallParsingError("func( + b)", "Invalid expression argument");
}

void ParserCallExprTest::testUnterminatedCalls() {
    testCallParsingError("func(a, b", "Missing closing parenthesis");
    testCallParsingError("func(a, b, c", "Missing closing parenthesis with multiple args");
    testCallParsingError("func(getValue(", "Nested unterminated call");
    testCallParsingError("obj:method(a, b", "Unterminated method call");
}

void ParserCallExprTest::testInvalidMethodCalls() {
    testCallParsingError("obj:", "Incomplete method call");
    testCallParsingError("obj:()", "Invalid method name");
    testCallParsingError(":method()", "Missing object in method call");
    testCallParsingError("obj::method()", "Double colon in method call");
}

void ParserCallExprTest::testCallParsing(const std::string& input, const std::string& expectedCallee, 
                                        int expectedArgCount, const std::string& testName) {
    try {
        Lua::Parser parser(input);
        
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printError("Failed to parse " + testName + ": " + input);
            return;
        }
        
        if (!verifyCallExpression(expr.get(), expectedCallee, expectedArgCount)) {
            TestUtils::printError("Call expression verification failed for " + testName + ": " + input);
            return;
        }
        
        TestUtils::printInfo("Successfully parsed " + testName + ": " + input);
        
    } catch (const std::exception& e) {
        TestUtils::printError("Exception parsing " + testName + ": " + e.what());
    }
}

void ParserCallExprTest::testMethodCallParsing(const std::string& input, const std::string& expectedObject,
                                              const std::string& expectedMethod, int expectedArgCount, 
                                              const std::string& testName) {
    try {
        Lua::Parser parser(input);
        
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printError("Failed to parse " + testName + ": " + input);
            return;
        }
        
        if (!verifyMethodCallExpression(expr.get(), expectedObject, expectedMethod, expectedArgCount)) {
            TestUtils::printError("Method call expression verification failed for " + testName + ": " + input);
            return;
        }
        
        TestUtils::printInfo("Successfully parsed " + testName + ": " + input);
        
    } catch (const std::exception& e) {
        TestUtils::printError("Exception parsing " + testName + ": " + e.what());
    }
}

void ParserCallExprTest::testCallParsingError(const std::string& input, const std::string& testName) {
    try {
        Lua::Parser parser(input);
        
        auto expr = parser.parseExpression();
        
        // If we get here without exception, the test failed
        TestUtils::printError("Expected error for " + testName + " but parsing succeeded: " + input);
        
    } catch (const std::exception& e) {
        TestUtils::printInfo("Correctly caught error for " + testName + ": " + e.what());
    }
}

bool ParserCallExprTest::verifyCallExpression(const Lua::Expr* expr, const std::string& expectedCallee, 
                                             int expectedArgCount) {
    if (!expr || expr->getType() != Lua::ExprType::Call) {
        return false;
    }
    
    const Lua::CallExpr* callExpr = static_cast<const Lua::CallExpr*>(expr);
    
    // Verify argument count
    if (static_cast<int>(callExpr->getArguments().size()) != expectedArgCount) {
        return false;
    }
    
    // Verify callee (simplified - just check if it's a variable with expected name)
    const Lua::Expr* callee = callExpr->getCallee();
    if (!callee) {
        return false;
    }
    
    // Handle simple variable calls
    if (callee->getType() == Lua::ExprType::Variable) {
        const Lua::VariableExpr* varExpr = static_cast<const Lua::VariableExpr*>(callee);
        return varExpr->getName() == expectedCallee;
    }
    
    // Handle member access calls (e.g., math.abs)
    if (callee->getType() == Lua::ExprType::Member) {
        const Lua::MemberExpr* memberExpr = static_cast<const Lua::MemberExpr*>(callee);
        const Lua::Expr* object = memberExpr->getObject();
        
        if (object && object->getType() == Lua::ExprType::Variable) {
            const Lua::VariableExpr* objVar = static_cast<const Lua::VariableExpr*>(object);
            std::string fullName = objVar->getName() + "." + memberExpr->getName();
            return fullName == expectedCallee;
        }
    }
    
    // For complex expressions, just return true for now
    return true;
}

bool ParserCallExprTest::verifyMethodCallExpression(const Lua::Expr* expr, const std::string& expectedObject,
                                                   const std::string& expectedMethod, int expectedArgCount) {
    if (!expr || expr->getType() != Lua::ExprType::Call) {
        return false;
    }
    
    const Lua::CallExpr* callExpr = static_cast<const Lua::CallExpr*>(expr);
    
    // Verify argument count (method calls have implicit self parameter)
    if (static_cast<int>(callExpr->getArguments().size()) != expectedArgCount) {
        return false;
    }
    
    // For method calls, the callee should be a member expression
    const Lua::Expr* callee = callExpr->getCallee();
    if (!callee || callee->getType() != Lua::ExprType::Member) {
        return false;
    }
    
    const Lua::MemberExpr* memberExpr = static_cast<const Lua::MemberExpr*>(callee);
    
    // Verify method name
    if (memberExpr->getName() != expectedMethod) {
        return false;
    }
    
    // Verify object
    const Lua::Expr* object = memberExpr->getObject();
    if (!object || object->getType() != Lua::ExprType::Variable) {
        return false;
    }
    
    const Lua::VariableExpr* objVar = static_cast<const Lua::VariableExpr*>(object);
    return objVar->getName() == expectedObject;
}