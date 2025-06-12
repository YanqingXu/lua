#include "conditional_compilation_test.hpp"
#include "../compiler/compiler.hpp"
#include "../parser/parser.hpp"
#include "../lexer/lexer.hpp"
#include "../vm/vm.hpp"
#include "../vm/state.hpp"
#include <iostream>
#include <cassert>

namespace Lua {
    void ConditionalCompilationTest::runAllTests() {
        std::cout << "Running Conditional Compilation Tests..." << std::endl;
        
        testSimpleIfStatement();
        testIfElseStatement();
        testNestedIfStatement();
        testShortCircuitAnd();
        testShortCircuitOr();
        testComplexConditions();
        
        std::cout << "All Conditional Compilation Tests Passed!" << std::endl;
    }
    
    void ConditionalCompilationTest::testSimpleIfStatement() {
        std::cout << "Testing simple if statement..." << std::endl;
        
        const char* code = R"(
            local x = 5
            if x > 3 then
                x = 10
            end
        )";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "Simple if statement compilation successful" << std::endl;
        } catch (const LuaException& e) {
            std::cerr << "Error in simple if test: " << e.what() << std::endl;
            throw;
        }
    }
    
    void ConditionalCompilationTest::testIfElseStatement() {
        std::cout << "Testing if-else statement..." << std::endl;
        
        const char* code = R"(
            local x = 2
            if x > 5 then
                x = 10
            else
                x = 1
            end
        )";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "If-else statement compilation successful" << std::endl;
        } catch (const LuaException& e) {
            std::cerr << "Error in if-else test: " << e.what() << std::endl;
            throw;
        }
    }
    
    void ConditionalCompilationTest::testNestedIfStatement() {
        std::cout << "Testing nested if statement..." << std::endl;
        
        const char* code = R"(
            local x = 5
            local y = 3
            if x > 3 then
                if y < 5 then
                    x = x + y
                else
                    x = x - y
                end
            end
        )";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "Nested if statement compilation successful" << std::endl;
        } catch (const LuaException& e) {
            std::cerr << "Error in nested if test: " << e.what() << std::endl;
            throw;
        }
    }
    
    void ConditionalCompilationTest::testShortCircuitAnd() {
        std::cout << "Testing short-circuit AND operator..." << std::endl;
        
        const char* code = R"(
            local x = 5
            local y = 3
            if x > 3 and y < 10 then
                x = x + y
            end
        )";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "Short-circuit AND compilation successful" << std::endl;
        } catch (const LuaException& e) {
            std::cerr << "Error in short-circuit AND test: " << e.what() << std::endl;
            throw;
        }
    }
    
    void ConditionalCompilationTest::testShortCircuitOr() {
        std::cout << "Testing short-circuit OR operator..." << std::endl;
        
        const char* code = R"(
            local x = 5
            local y = 3
            if x < 3 or y > 1 then
                x = x * y
            end
        )";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "Short-circuit OR compilation successful" << std::endl;
        } catch (const LuaException& e) {
            std::cerr << "Error in short-circuit OR test: " << e.what() << std::endl;
            throw;
        }
    }
    
    void ConditionalCompilationTest::testComplexConditions() {
        std::cout << "Testing complex conditions..." << std::endl;
        
        const char* code = R"(
            local x = 5
            local y = 3
            local z = 7
            if (x > 3 and y < 5) or z > 10 then
                x = x + y + z
            else
                if x < y then
                    x = y
                else
                    x = z
                end
            end
        )";
        
        try {
            Parser parser(code);
            auto statements = parser.parse();
            
            Compiler compiler;
            auto function = compiler.compile(statements);
            
            assert(function != nullptr);
            std::cout << "Complex conditions compilation successful" << std::endl;
        } catch (const LuaException& e) {
            std::cerr << "Error in complex conditions test: " << e.what() << std::endl;
            throw;
        }
    }
}