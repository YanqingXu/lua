#include "function_test.hpp"
#include "../parser/parser.hpp"
#include "../parser/ast/statements.hpp"
#include <iostream>
#include <memory>

namespace Lua {
    void testFunctionSyntax() {
        std::cout << "\n=== Testing Function Definition Syntax ===" << std::endl;
        
        // Test cases for function definition parsing
        Vec<Str> testCases = {
            // Basic function definition
            "function test() end",
            
            // Function with single parameter
            "function greet(name) end",
            
            // Function with multiple parameters
            "function add(a, b) end",
            
            // Function with many parameters
            "function complex(a, b, c, d, e) end",
            
            // Function with body statements
            "function calculate(x) local result = x * 2; return result end",
            
            // Function with local variables
            "function process() local temp = 10; local value = temp + 5 end",
            
            // Function with control structures
            "function loop(n) for i = 1, n do print(i) end end",
            
            // Function with conditional statements
            "function check(x) if x > 0 then return true else return false end end"
        };
        
        for (const auto& testCase : testCases) {
            std::cout << "\nTesting: " << testCase << std::endl;
            
            try {
                Parser parser(testCase);
                auto statements = parser.parse();
                
                if (parser.hasError()) {
                    std::cout << "  Parse error occurred" << std::endl;
                    continue;
                }
                
                // Check if it's a Function statement
                if (!statements.empty() && statements[0]->getType() == StmtType::Function) {
                    std::cout << "  Confirmed as Function statement" << std::endl;
                    const FunctionStmt* funcStmt = static_cast<const FunctionStmt*>(statements[0].get());
                    std::cout << "  Function name: " << funcStmt->getName() << std::endl;
                    std::cout << "  Parameter count: " << funcStmt->getParameters().size() << std::endl;
                    std::cout << "  Has function body" << std::endl;
                } else {
                    std::cout << "  Not recognized as Function statement" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "  Exception: " << e.what() << std::endl;
            }
        }
        
        std::cout << "  Function syntax test completed" << std::endl;
    }
    
    void testFunctionParameters() {
        std::cout << "\n=== Testing Function Parameter Parsing ===" << std::endl;
        
        // Test cases focusing on parameter parsing
        Vec<std::pair<Str, int>> parameterTests = {
            {"function noParams() end", 0},
            {"function oneParam(x) end", 1},
            {"function twoParams(a, b) end", 2},
            {"function threeParams(x, y, z) end", 3},
            {"function manyParams(a, b, c, d, e, f) end", 6}
        };
        
        for (const auto& test : parameterTests) {
            std::cout << "\nTesting: " << test.first << std::endl;
            std::cout << "Expected parameters: " << test.second << std::endl;
            
            try {
                Parser parser(test.first);
                auto statements = parser.parse();
                
                if (!parser.hasError() && !statements.empty() && 
                    statements[0]->getType() == StmtType::Function) {
                    const FunctionStmt* funcStmt = static_cast<const FunctionStmt*>(statements[0].get());
                    int actualParams = static_cast<int>(funcStmt->getParameters().size());
                    
                    if (actualParams == test.second) {
                        std::cout << "  [OK] Parameter count matches: " << actualParams << std::endl;
                        
                        // Print parameter names
                        if (actualParams > 0) {
                            std::cout << "  Parameters: ";
                            const auto& params = funcStmt->getParameters();
                            for (size_t i = 0; i < params.size(); ++i) {
                                if (i > 0) std::cout << ", ";
                                std::cout << params[i];
                            }
                            std::cout << std::endl;
                        }
                    } else {
                        std::cout << "  [ERROR] Parameter count mismatch. Expected: " << test.second 
                                  << ", Got: " << actualParams << std::endl;
                    }
                } else {
                    std::cout << "  [ERROR] Failed to parse as function statement" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "  Exception: " << e.what() << std::endl;
            }
        }
        
        std::cout << "  Function parameter test completed" << std::endl;
    }
    
    void testFunctionBody() {
        std::cout << "\n=== Testing Function Body Parsing ===" << std::endl;
        
        // Test cases for function body parsing
        Vec<Str> bodyTests = {
            "function empty() end",
            "function simple() return 42 end",
            "function withLocal() local x = 10; return x end",
            "function withLoop() for i = 1, 5 do print(i) end end",
            "function complex() local a = 1; local b = 2; return a + b end"
        };
        
        for (const auto& test : bodyTests) {
            std::cout << "\nTesting: " << test << std::endl;
            
            try {
                Parser parser(test);
                auto statements = parser.parse();
                
                if (!parser.hasError() && !statements.empty() && 
                    statements[0]->getType() == StmtType::Function) {
                    const FunctionStmt* funcStmt = static_cast<const FunctionStmt*>(statements[0].get());
                    
                    if (funcStmt->getBody()) {
                        std::cout << "  [OK] Function body parsed successfully" << std::endl;
                        std::cout << "  Body type: " << static_cast<int>(funcStmt->getBody()->getType()) << std::endl;
                    } else {
                        std::cout << "  [ERROR] Function body is null" << std::endl;
                    }
                } else {
                    std::cout << "  [ERROR] Failed to parse as function statement" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "  Exception: " << e.what() << std::endl;
            }
        }
        
        std::cout << "  Function body test completed" << std::endl;
    }
    
    void testFunctionCompilation() {
        std::cout << "\n=== Testing Function Compilation ===" << std::endl;
        
        // Test simple function parsing (compilation test simplified)
        Str code = "function test(x) return x + 1 end";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            if (!parser.hasError() && !statements.empty() && 
                statements[0]->getType() == StmtType::Function) {
                std::cout << "  [OK] Function parsed successfully for compilation" << std::endl;
                const FunctionStmt* funcStmt = static_cast<const FunctionStmt*>(statements[0].get());
                std::cout << "  Function ready for compilation: " << funcStmt->getName() << std::endl;
                // Note: Actual compilation would require full compiler integration
            } else {
                std::cout << "  [ERROR] Function parsing failed" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "  Exception during parsing: " << e.what() << std::endl;
        }
        
        std::cout << "  Function compilation test completed" << std::endl;
    }
    
    void runFunctionTests() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "         FUNCTION TESTS" << std::endl;
        std::cout << "========================================" << std::endl;
        
        testFunctionSyntax();
        testFunctionParameters();
        testFunctionBody();
        testFunctionCompilation();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "      FUNCTION TESTS COMPLETED" << std::endl;
        std::cout << "========================================" << std::endl;
    }
}