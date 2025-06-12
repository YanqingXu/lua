#include "if_statement_test.hpp"
#include "../compiler/compiler.hpp"
#include "../parser/parser.hpp"
#include "../lexer/lexer.hpp"
#include "../vm/vm.hpp"
#include "../vm/state_factory.hpp"
#include <iostream>
#include <cassert>

namespace Lua {
    void IfStatementTest::runAllTests() {
        std::cout << "Running If Statement Tests..." << std::endl;
        
        testSimpleIfStatement();
        testIfElseStatement();
        testNestedIfStatement();
        testIfWithComplexCondition();
        
        std::cout << "All If Statement tests passed!" << std::endl;
    }
    
    void IfStatementTest::testSimpleIfStatement() {
        std::cout << "Testing simple if statement..." << std::endl;
        
        // Test: if true then x = 1 end
        Str code = "if true then x = 1 end";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "Simple if statement compilation successful" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Simple if test failed: " << e.what() << std::endl;
            throw;
        }
    }
    
    void IfStatementTest::testIfElseStatement() {
        std::cout << "Testing if-else statement..." << std::endl;
        
        // Test: if false then x = 1 else x = 2 end
        Str code = "if false then x = 1 else x = 2 end";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "If-else statement compilation successful" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "If-else test failed: " << e.what() << std::endl;
            throw;
        }
    }
    
    void IfStatementTest::testNestedIfStatement() {
        std::cout << "Testing nested if statement..." << std::endl;
        
        // Test: if true then if false then x = 1 else x = 2 end end
        Str code = "if true then if false then x = 1 else x = 2 end end";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "Nested if statement compilation successful" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Nested if test failed: " << e.what() << std::endl;
            throw;
        }
    }
    
    void IfStatementTest::testIfWithComplexCondition() {
        std::cout << "Testing if with complex condition..." << std::endl;
        
        // Test: if x == 5 then y = 10 else y = 20 end
        Str code = "if x == 5 then y = 10 else y = 20 end";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "Complex condition if statement compilation successful" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Complex condition if test failed: " << e.what() << std::endl;
            throw;
        }
    }
    
    void IfStatementTest::testIfStatementExecution() {
        std::cout << "Testing if statement execution..." << std::endl;
        
        // Test execution with VM
        Str code = "local x; if true then x = 42 else x = 0 end";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            
            // Create VM and execute
            auto state = make_gc_state();
            VM vm(state.get());
            
            Value result = vm.execute(function);
            std::cout << "If statement execution successful" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "If execution test failed: " << e.what() << std::endl;
            throw;
        }
    }
}