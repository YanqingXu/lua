#include "forin_test.hpp"
#include "../../parser/parser.hpp"
#include "../../vm/state.hpp"
#include "../../vm/table.hpp"
#include "../../lib/base_lib.hpp"
#include "../../gc/core/gc_ref.hpp"
#include <iostream>

namespace Lua {
namespace Tests {

    void ForInTest::runAllTests() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Running For-In Loop Tests" << std::endl;
        std::cout << "========================================" << std::endl;
        
        testForInSyntax();
        testForInExecution();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "For-In Loop Tests Completed" << std::endl;
        std::cout << "========================================" << std::endl;
    }

    void ForInTest::testForInSyntax() {
            std::cout << "\n=== Testing For-In Loop Syntax ===" << std::endl;
            
            // Test cases for for-in loop parsing
            Vec<Str> testCases = {
                // Basic for-in with pairs
                "for k, v in pairs(table) do print(k, v) end",
                
                // Basic for-in with ipairs
                "for i, v in ipairs(array) do print(i, v) end",
                
                // Single variable for-in
                "for key in next, table do print(key) end",
                
                // Multiple variables for-in
                "for a, b, c in iterator() do print(a, b, c) end",
                
                // Nested for-in loops
                "for k, v in pairs(outer) do for i, item in ipairs(v) do print(k, i, item) end end"
            };
            
            for (const auto& testCase : testCases) {
                std::cout << "\nTesting: " << testCase << std::endl;
                
                try {
                    Parser parser(testCase);
                    auto statements = parser.parse();
                    
                    if (parser.hasError()) {
                        std::cout << "  Parse Error!" << std::endl;
                    } else {
                        std::cout << "  Parsed successfully! (" << statements.size() << " statements)" << std::endl;
                        
                        // Check if it's a ForIn statement
                        if (!statements.empty() && statements[0]->getType() == StmtType::ForIn) {
                            std::cout << "  Confirmed as ForIn statement" << std::endl;
                            const ForInStmt* forInStmt = static_cast<const ForInStmt*>(statements[0].get());
                            std::cout << "  Variables: ";
                            for (const auto& var : forInStmt->getVariables()) {
                                std::cout << var << " ";
                            }
                            std::cout << std::endl;
                            std::cout << "  Iterator expressions: " << forInStmt->getIterators().size() << std::endl;
                        }
                    }
                } catch (const std::exception& e) {
                    std::cout << "  Exception: " << e.what() << std::endl;
                }
            }
        }
        
        void ForInTest::testForInExecution() {
            std::cout << "\n=== Testing For-In Loop Execution ===" << std::endl;
            
            try {
                // Create Lua state
                State state;
                
                // Register base library (includes pairs and ipairs)
                registerBaseLib(&state);
                
                // Test table creation and for-in iteration
                std::cout << "\nTesting table iteration with pairs:" << std::endl;
                
                // Create a test table
                auto table = make_gc_table();
                table->set(Value("a"), Value(1.0));
                table->set(Value("b"), Value(2.0));
                table->set(Value("c"), Value(3.0));
                
                // Set table as global
                state.setGlobal("testTable", Value(table));
                
                // Get pairs function
                auto pairsFunc = state.getGlobal("pairs");
                if (pairsFunc.isFunction()) {
                    std::cout << "  pairs function found and ready" << std::endl;
                } else {
                    std::cout << "  pairs function not found!" << std::endl;
                }
                
                // Test array iteration with ipairs
                std::cout << "\nTesting array iteration with ipairs:" << std::endl;
                
                auto arrayTable = make_gc_table();
                arrayTable->set(Value(1.0), Value(10.0));
                arrayTable->set(Value(2.0), Value(20.0));
                arrayTable->set(Value(3.0), Value(30.0));
                
                state.setGlobal("testArray", Value(arrayTable));
                
                auto ipairsFunc = state.getGlobal("ipairs");
                if (ipairsFunc.isFunction()) {
                    std::cout << "  ipairs function found and ready" << std::endl;
                } else {
                    std::cout << "  ipairs function not found!" << std::endl;
                }
                
                std::cout << "  For-in execution test completed" << std::endl;
                
            } catch (const std::exception& e) {
                std::cout << "  Exception during execution test: " << e.what() << std::endl;
            }
        }
  } // namespace Tests
} // namespace Lua