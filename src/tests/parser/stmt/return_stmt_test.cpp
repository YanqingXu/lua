#include "return_stmt_test.hpp"
#include "../../../parser/parser.hpp"
#include "../../../lexer/lexer.hpp"
#include "../../../test_framework/core/test_utils.hpp"

namespace Lua {
    void ReturnStmtTest::runAllTests() {
        TestUtils::printTestGroupHeader("Return Statement Tests");
        
        testEmptyReturn();
        testSingleReturn();
        testMultipleReturn();
        testReturnWithExpressions();
        testReturnSyntaxErrors();
        
        TestUtils::printTestGroupFooter("Return Statement Tests");
    }
    
    void ReturnStmtTest::testEmptyReturn() {
        testReturnParsing("return;", 0, "Empty return statement");
        testReturnParsing("return", 0, "Return without semicolon");
    }
    
    void ReturnStmtTest::testSingleReturn() {
        testReturnParsing("return 42;", 1, "Return single number");
        testReturnParsing("return \"hello\";", 1, "Return single string");
        testReturnParsing("return x;", 1, "Return single variable");
        testReturnParsing("return x + y;", 1, "Return single expression");
        testReturnParsing("return func();", 1, "Return function call");
    }
    
    void ReturnStmtTest::testMultipleReturn() {
        testReturnParsing("return 1, 2;", 2, "Return two numbers");
        testReturnParsing("return x, y, z;", 3, "Return three variables");
        testReturnParsing("return 1, \"hello\", true;", 3, "Return mixed types");
        testReturnParsing("return a + b, c * d;", 2, "Return two expressions");
        testReturnParsing("return func1(), func2(), func3();", 3, "Return three function calls");
    }
    
    void ReturnStmtTest::testReturnWithExpressions() {
        testReturnParsing("return x.field, y[index];", 2, "Return member and index access");
        testReturnParsing("return {a=1}, {b=2};", 2, "Return two table constructors");
        testReturnParsing("return (x + y), (a * b);", 2, "Return parenthesized expressions");
        testReturnParsing("return f(x), g(y, z);", 2, "Return function calls with different arguments");
    }
    
    void ReturnStmtTest::testReturnSyntaxErrors() {
        testReturnParsingError("return ,;", "Leading comma in return statement");
        testReturnParsingError("return 1,;", "Trailing comma in return statement");
        testReturnParsingError("return 1,,2;", "Double comma in return statement");
        testReturnParsingError("return 1 2;", "Missing comma between return values");
    }
    
    void ReturnStmtTest::testReturnParsing(const Str& code, size_t expectedValueCount, const Str& description) {
        try {
            Lexer lexer(code);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            
            auto statements = parser.parse();
            
            bool success = false;
            if (!statements.empty()) {
                auto* returnStmt = dynamic_cast<const ReturnStmt*>(statements[0].get());
                if (returnStmt) {
                    success = (returnStmt->getValueCount() == expectedValueCount);
                    
                    if (!success) {
                        std::cerr << "Expected " << expectedValueCount << " return values, got " 
                                  << returnStmt->getValueCount() << std::endl;
                    }
                }
            }
            
            TestUtils::printTestResult(description, success);
        } catch (const std::exception& e) {
            TestUtils::printTestResult(description + " (Exception: " + e.what() + ")", false);
        }
    }
    
    void ReturnStmtTest::testReturnParsingError(const Str& code, const Str& description) {
        try {
            Lexer lexer(code);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            
            auto statements = parser.parse();
            
            // If we get here without exception, the test failed
            TestUtils::printTestResult(description + " (Expected error but parsing succeeded)", false);
        } catch (const std::exception& e) {
            // Expected to throw an exception
            TestUtils::printTestResult(description, true);
        }
    }
}