#include "test_expr.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

void ExprTestSuite::runAllTests() {
    TestUtils::printSectionHeader("EXPRESSION PARSER TEST SUITE");
    TestUtils::printInfo("Running all expression parsing tests...");
    
    try {
        // Run all expression test suites using standardized macros
        //RUN_TEST_SUITE(LiteralExprTest);
        //RUN_TEST_SUITE(VariableExprTest);
        //RUN_TEST_SUITE(UnaryExprTest);
        RUN_TEST_SUITE(BinaryExprTest);
        //RUN_TEST_SUITE(CallExprTest);
        //RUN_TEST_SUITE(TableExprTest);
        //RUN_TEST_SUITE(MemberExprTest);
        
        TestUtils::printInfo("ALL EXPRESSION TESTS COMPLETED SUCCESSFULLY");
        
    } catch (const std::exception& e) {
        TestUtils::printError("Expression test suite failed: " + std::string(e.what()));
        throw;
    }
    
    TestUtils::printSectionFooter();
}

} // namespace Tests
} // namespace Lua