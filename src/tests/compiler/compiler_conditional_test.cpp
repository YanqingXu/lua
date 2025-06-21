#include "compiler_conditional_test.hpp"
#include "../../compiler/compiler.hpp"
#include "../../parser/parser.hpp"
#include "../../lexer/lexer.hpp"
#include "../../vm/vm.hpp"
#include "../../vm/state.hpp"
#include <iostream>
#include <cassert>

namespace Lua::Tests {
    
    void CompilerConditionalTest::testSimpleIfStatement() {
        TestUtils::printInfo("Testing simple if statement compilation...");
        
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
            TestUtils::printTestResult("Simple if statement compilation", true);
        } catch (const LuaException& e) {
            TestUtils::printError("Simple if statement compilation failed: " + std::string(e.what()));
            throw;
        }
    }
    
    void CompilerConditionalTest::testIfElseStatement() {
        TestUtils::printInfo("Testing if-else statement compilation...");
        
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
            TestUtils::printTestResult("If-else statement compilation", true);
        } catch (const LuaException& e) {
            TestUtils::printError("If-else statement compilation failed: " + std::string(e.what()));
            throw;
        }
    }
    
    void CompilerConditionalTest::testNestedIfStatement() {
        TestUtils::printInfo("Testing nested if statement compilation...");
        
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
            TestUtils::printTestResult("Nested if statement compilation", true);
        } catch (const LuaException& e) {
            TestUtils::printError("Nested if statement compilation failed: " + std::string(e.what()));
            throw;
        }
    }
    
    void CompilerConditionalTest::testShortCircuitAnd() {
        TestUtils::printInfo("Testing short-circuit AND operator compilation...");
        
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
            TestUtils::printTestResult("Short-circuit AND compilation", true);
        } catch (const LuaException& e) {
            TestUtils::printError("Short-circuit AND compilation failed: " + std::string(e.what()));
            throw;
        }
    }
    
    void CompilerConditionalTest::testShortCircuitOr() {
        TestUtils::printInfo("Testing short-circuit OR operator compilation...");
        
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
            TestUtils::printTestResult("Short-circuit OR compilation", true);
        } catch (const LuaException& e) {
            TestUtils::printError("Short-circuit OR compilation failed: " + std::string(e.what()));
            throw;
        }
    }
    
    void CompilerConditionalTest::testComplexConditionCombinations() {
        TestUtils::printInfo("Testing complex conditional combinations...");
        
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
            TestUtils::printTestResult("Complex conditional combinations compilation", true);
        } catch (const LuaException& e) {
            TestUtils::printError("Complex conditions compilation failed: " + std::string(e.what()));
            throw;
        }
    }

} // namespace Lua::Tests