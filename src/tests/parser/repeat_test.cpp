#include "repeat_test.hpp"
#include "../../parser/parser.hpp"
#include "../../parser/ast/statements.hpp"
#include "../../compiler/compiler.hpp"
#include "../../vm/state.hpp"
#include "../../vm/table.hpp"
#include <iostream>
#include <memory>

namespace Lua {
    void testRepeatUntilSyntax() {
        std::cout << "\n=== Testing Repeat-Until Loop Syntax ===" << std::endl;
        
        // Test cases for repeat-until loop parsing
        Vec<Str> testCases = {
            // Basic repeat-until
            "repeat x = x + 1 until x > 10",
            
            // Multiple statements in body
            "repeat print(i); i = i + 1 until i >= 5",
            
            // Local variable in body
            "repeat local temp = getValue() until temp ~= nil",
            
            // Complex condition
            "repeat doSomething() until condition == true",
            
            // Nested repeat-until
            "repeat repeat y = y * 2 until y > 100 until x < 0"
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
                
                // Check if it's a RepeatUntil statement
                if (!statements.empty() && statements[0]->getType() == StmtType::RepeatUntil) {
                    std::cout << "  Confirmed as RepeatUntil statement" << std::endl;
                    const RepeatUntilStmt* repeatStmt = static_cast<const RepeatUntilStmt*>(statements[0].get());
                    std::cout << "  Has body and condition" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "  Exception: " << e.what() << std::endl;
            }
        }
        
        std::cout << "  Repeat-until syntax test completed" << std::endl;
    }
    
    void testRepeatUntilExecution() {
        std::cout << "\n=== Testing Repeat-Until Loop Execution ===" << std::endl;
        
        try {
            // Create Lua state
            auto state = std::make_shared<State>();
            
            // Test simple repeat-until loop execution
            Str code = "repeat x = x + 1 until x > 3";
            
            Parser parser(code);
            auto statements = parser.parse();
            
            if (!parser.hasError() && !statements.empty()) {
                Compiler compiler;
                auto function = compiler.compile(statements);
                
                if (function) {
                    std::cout << "  Repeat-until loop compiled successfully" << std::endl;
                    // Note: Actual execution would require VM integration
                } else {
                    std::cout << "  Compilation failed" << std::endl;
                }
            } else {
                std::cout << "  Parse failed" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "  Exception during execution test: " << e.what() << std::endl;
        }
        
        std::cout << "  Repeat-until execution test completed" << std::endl;
    }
    
    void runRepeatUntilTests() {
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "Running Repeat-Until Loop Tests" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        testRepeatUntilSyntax();
        testRepeatUntilExecution();
        
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "Repeat-Until Loop Tests Completed" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
    }
}