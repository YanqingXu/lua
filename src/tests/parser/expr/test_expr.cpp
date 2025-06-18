#include "test_expr.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

void ExprTestSuite::runAllTests() {
    try {
        // Run all expression test suites using standardized macros
        RUN_TEST_GROUP("Binary Expression Tests", BinaryExprTest::runAllTests);

        //RUN_TEST_GROUP("Unary Expression Tests", UnaryExprTest);
        //RUN_TEST_GROUP("Call Expression Tests", CallExprTest);
        //RUN_TEST_GROUP("Table Expression Tests", TableExprTest);
        //RUN_TEST_GROUP("Member Expression Tests", MemberExprTest);
        //RUN_TEST_GROUP("Literal Expression Tests", LiteralExprTest);
        //RUN_TEST_GROUP("Variable Expression Tests", VariableExprTest);
        
    } catch (const std::exception& e) {
        TestUtils::printError("Expression test suite failed: " + std::string(e.what()));
        throw;
    }
}

} // namespace Tests
} // namespace Lua