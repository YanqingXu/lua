#include "error_handling_suite.hpp"

namespace Lua {
namespace Tests {

void ErrorHandlingSuite::runAllTests() {
    printSuiteHeader();
    
    try {
        // Run error handling tests for each module
        runLexerErrorTests();
        runParserErrorTests();
        runCompilerErrorTests();
        runVMErrorTests();
        runGCErrorTests();
        
        // Run integration tests
        runIntegrationErrorTests();
        
        printSuiteFooter();
        
    } catch (const std::exception& e) {
        TestUtils::printError("Error Handling Suite failed: " + std::string(e.what()));
        printSuiteFooter();
        throw;
    }
}

void ErrorHandlingSuite::runLexerErrorTests() {
    printModuleHeader("Lexer Error Handling");
    
    try {
        LexerErrorTestSuite::runAllTests();
        TestUtils::printInfo("Lexer error handling tests completed successfully");
        
    } catch (const std::exception& e) {
        TestUtils::printError("Lexer error tests failed: " + std::string(e.what()));
        throw;
    }
    
    printModuleFooter("Lexer Error Handling");
}

void ErrorHandlingSuite::runParserErrorTests() {
    printModuleHeader("Parser Error Recovery");
    
    try {
        ParserErrorRecoveryTest::runAllTests();
        TestUtils::printInfo("Parser error recovery tests completed successfully");
        
    } catch (const std::exception& e) {
        TestUtils::printError("Parser error tests failed: " + std::string(e.what()));
        throw;
    }
    
    printModuleFooter("Parser Error Recovery");
}

void ErrorHandlingSuite::runCompilerErrorTests() {
    printModuleHeader("Compiler Error Detection");
    
    try {
        CompilerErrorTest::runAllTests();
        TestUtils::printInfo("Compiler error detection tests completed successfully");
        
    } catch (const std::exception& e) {
        TestUtils::printError("Compiler error tests failed: " + std::string(e.what()));
        throw;
    }
    
    printModuleFooter("Compiler Error Detection");
}

void ErrorHandlingSuite::runVMErrorTests() {
    printModuleHeader("VM Runtime Error Handling");
    
    try {
        VMErrorTest::runAllTests();
        TestUtils::printInfo("VM runtime error handling tests completed successfully");
        
    } catch (const std::exception& e) {
        TestUtils::printError("VM error tests failed: " + std::string(e.what()));
        throw;
    }
    
    printModuleFooter("VM Runtime Error Handling");
}

void ErrorHandlingSuite::runGCErrorTests() {
    printModuleHeader("GC Error Handling");
    
    try {
        GCErrorTest::runAllTests();
        TestUtils::printInfo("GC error handling tests completed successfully");
        
    } catch (const std::exception& e) {
        TestUtils::printError("GC error tests failed: " + std::string(e.what()));
        throw;
    }
    
    printModuleFooter("GC Error Handling");
}

void ErrorHandlingSuite::runIntegrationErrorTests() {
    printModuleHeader("Integration Error Tests");
    
    try {
        RUN_TEST_GROUP("Error Propagation", testErrorPropagation);
        RUN_TEST_GROUP("System Error Recovery", testSystemErrorRecovery);
        RUN_TEST_GROUP("Error Reporting", testErrorReporting);
        
        TestUtils::printInfo("Integration error tests completed successfully");
        
    } catch (const std::exception& e) {
        TestUtils::printError("Integration error tests failed: " + std::string(e.what()));
        throw;
    }
    
    printModuleFooter("Integration Error Tests");
}

void ErrorHandlingSuite::testErrorPropagation() {
    TestUtils::printLevelHeader(TestUtils::TestLevel::SUITE, "Error Propagation Tests", 
                               "Testing error propagation between modules");
    
    // Test 1: Lexer error propagation to parser
    RUN_TEST("Lexer to Parser Error Propagation", []() {
        std::string invalidSource = "\x00\x01\x02 invalid tokens";
        
        try {
            Lexer lexer(invalidSource);
            Parser parser(invalidSource);
            
            auto ast = parser.parseExpression();
            return ast == nullptr; // Should fail due to lexer errors
            
        } catch (const std::exception& e) {
            return true; // Exception indicates proper error propagation
        }
    });
    
    // Test 2: Parser error propagation to compiler
    RUN_TEST("Parser to Compiler Error Propagation", []() {
        std::string invalidSyntax = "local x = + * /";
        
        try {
            Lexer lexer(invalidSyntax);
            Parser parser(invalidSyntax);
            
            auto ast = parser.parseExpression();
            if (ast == nullptr) {
                return true; // Parser correctly failed
            }
            
            Compiler compiler;
            auto bytecode = compiler.compile(ast.get());
            return bytecode == nullptr; // Should fail due to invalid AST
            
        } catch (const std::exception& e) {
            return true; // Exception indicates proper error propagation
        }
    });
    
    // Test 3: Compiler error propagation to VM
    RUN_TEST("Compiler to VM Error Propagation", []() {
        std::string semanticError = "return undefinedVariable";
        
        try {
            Lexer lexer(semanticError);
            Parser parser(semanticError);
            
            auto ast = parser.parseExpression();
            if (ast != nullptr) {
                Compiler compiler;
                auto bytecode = compiler.compile(ast.get());
                
                if (bytecode == nullptr) {
                    return true; // Compiler correctly failed
                }
                
                VM vm;
                auto result = vm.execute(bytecode.get());
                return false; // Should not reach here with undefined variable
            }
            
            return true; // Parser failed as expected
            
        } catch (const std::exception& e) {
            return true; // Exception indicates proper error propagation
        }
    });
    
    TestUtils::printLevelFooter(TestUtils::TestLevel::SUITE, "Error Propagation Tests completed");
}

void ErrorHandlingSuite::testSystemErrorRecovery() {
    TestUtils::printLevelHeader(TestUtils::TestLevel::SUITE, "System Error Recovery Tests", 
                               "Testing system-wide error recovery mechanisms");
    
    // Test 1: Recovery after lexer errors
    RUN_TEST("Recovery After Lexer Errors", []() {
        try {
            // First, cause a lexer error
            std::string invalidSource = "\x00 invalid";
            Lexer lexer1(invalidSource);
            
            try {
                auto tokens = lexer1.tokenize();
            } catch (const std::exception& e) {
                // Expected error
            }
            
            // Then, try to use lexer with valid input
            std::string validSource = "local x = 42";
            Lexer lexer2(validSource);
            auto tokens = lexer2.tokenize();
            
            return !tokens.empty(); // Should successfully tokenize
            
        } catch (const std::exception& e) {
            return false;
        }
    });
    
    // Test 2: Recovery after parser errors
    RUN_TEST("Recovery After Parser Errors", []() {
        try {
            // First, cause a parser error
            std::string invalidSyntax = "local x = + *";
            Parser parser1(invalidSyntax);
            
            try {
                auto ast = parser1.parseExpression();
            } catch (const std::exception& e) {
                // Expected error
            }
            
            // Then, try to parse valid syntax
            std::string validSyntax = "local y = 42";
            Parser parser2(validSyntax);
            auto ast = parser2.parseExpression();
            
            return ast != nullptr; // Should successfully parse
            
        } catch (const std::exception& e) {
            return false;
        }
    });
    
    // Test 3: Recovery after VM errors
    RUN_TEST("Recovery After VM Errors", []() {
        try {
            VM vm;
            
            // First, cause a VM error
            std::string errorCode = "return 10 / 0";
            Lexer lexer1(errorCode);
            Parser parser1(errorCode);
            
            auto ast1 = parser1.parseExpression();
            if (ast1 != nullptr) {
                Compiler compiler1;
                auto bytecode1 = compiler1.compile(ast1.get());
                
                if (bytecode1 != nullptr) {
                    try {
                        auto result1 = vm.execute(bytecode1.get());
                    } catch (const std::exception& e) {
                        // Expected error
                    }
                }
            }
            
            // Then, try to execute valid code
            std::string validCode = "return 42";
            Lexer lexer2(validCode);
            Parser parser2(validCode);
            
            auto ast2 = parser2.parseExpression();
            if (ast2 != nullptr) {
                Compiler compiler2;
                auto bytecode2 = compiler2.compile(ast2.get());
                
                if (bytecode2 != nullptr) {
                    auto result2 = vm.execute(bytecode2.get());
                    return true; // Should successfully execute
                }
            }
            
            return false;
            
        } catch (const std::exception& e) {
            return false;
        }
    });
    
    TestUtils::printLevelFooter(TestUtils::TestLevel::SUITE, "System Error Recovery Tests completed");
}

void ErrorHandlingSuite::testErrorReporting() {
    TestUtils::printLevelHeader(TestUtils::TestLevel::SUITE, "Error Reporting Tests", 
                               "Testing consistency and quality of error messages");
    
    // Test 1: Error message consistency
    RUN_TEST("Error Message Consistency", []() {
        // Test that similar errors produce consistent messages
        std::vector<std::string> invalidInputs = {
            "\x00",
            "\x01",
            "\xFF"
        };
        
        std::vector<std::string> errorMessages;
        
        for (const auto& input : invalidInputs) {
            try {
                Lexer lexer(input);
                auto tokens = lexer.tokenize();
            } catch (const std::exception& e) {
                errorMessages.push_back(e.what());
            }
        }
        
        // Check that all error messages contain similar patterns
        // (This is a simplified check - real implementation would be more sophisticated)
        return errorMessages.size() == invalidInputs.size();
    });
    
    // Test 2: Error message informativeness
    RUN_TEST("Error Message Informativeness", []() {
        try {
            std::string source = "local x = undefinedVariable";
            Lexer lexer(source);
            Parser parser(source);
            
            auto ast = parser.parseExpression();
            if (ast != nullptr) {
                Compiler compiler;
                auto bytecode = compiler.compile(ast.get());
                // If compilation fails, check that error message is informative
                return bytecode == nullptr; // Expected to fail
            }
            
            return true; // Parser failed, which is also acceptable
            
        } catch (const std::exception& e) {
            std::string errorMsg = e.what();
            // Check that error message contains useful information
            return !errorMsg.empty() && errorMsg.length() > 10;
        }
    });
    
    // Test 3: Error location reporting
    RUN_TEST("Error Location Reporting", []() {
        try {
            std::string source = "local x = 1\nlocal y = +";
            Parser parser(source);
            
            auto ast = parser.parseExpression();
            return ast == nullptr; // Should fail due to syntax error
            
        } catch (const std::exception& e) {
            std::string errorMsg = e.what();
            // In a real implementation, we'd check for line/column information
            return !errorMsg.empty();
        }
    });
    
    TestUtils::printLevelFooter(TestUtils::TestLevel::SUITE, "Error Reporting Tests completed");
}

void ErrorHandlingSuite::printSuiteHeader() {
    TestUtils::printLevelHeader(TestUtils::TestLevel::MODULE, "Error Handling Test Suite", 
                               "Comprehensive error handling tests across all modules");
}

void ErrorHandlingSuite::printSuiteFooter() {
    TestUtils::printLevelFooter(TestUtils::TestLevel::MODULE, "Error Handling Test Suite completed");
}

void ErrorHandlingSuite::printModuleHeader(const std::string& moduleName) {
    TestUtils::printLevelHeader(TestUtils::TestLevel::SUITE, moduleName, 
                               "Testing error handling in " + moduleName);
}

void ErrorHandlingSuite::printModuleFooter(const std::string& moduleName) {
    TestUtils::printLevelFooter(TestUtils::TestLevel::SUITE, moduleName + " completed");
}

} // namespace Tests
} // namespace Lua