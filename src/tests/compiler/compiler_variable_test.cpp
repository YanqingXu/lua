#include "compiler_variable_test.hpp"
#include "../../compiler/expression_compiler.hpp"
#include "../../compiler/compiler.hpp"
#include "../../vm/value.hpp"
#include "../../parser/ast/expressions.hpp"
#include <cassert>
#include <iostream>

namespace Lua {
    namespace Tests {
        void CompilerVariableTest::runAllTests() {
            std::cout << "=== Running Variable Compiler Tests ===" << std::endl;
            
            testLocalVariableAccess();
            testGlobalVariableAccess();
            testVariableResolution();
            testScopeHandling();
            testRegisterAllocation();
            testInstructionGeneration();
            testErrorHandling();
            
            std::cout << "All Variable Compiler Tests Passed!" << std::endl;
        }
        
        void CompilerVariableTest::testLocalVariableAccess() {
            std::cout << "Testing local variable access..." << std::endl;
            
            Compiler compiler;
            
            // Add a local variable
            compiler.beginScope();
            int localSlot = compiler.allocReg();
            compiler.addLocal("x", localSlot);
            
            // Test accessing the local variable
            VariableExpr varExpr("x");
            int reg = compiler.compileExpr(&varExpr);
            
            // Local variable should return its slot directly
            assert(reg == localSlot);
            
            // No instruction should be generated for local access
            assert(compiler.getCodeSize() == 0);
            
            compiler.endScope();
            
            std::cout << "[OK] Local variable access test passed" << std::endl;
        }
        
        void CompilerVariableTest::testGlobalVariableAccess() {
            std::cout << "Testing global variable access..." << std::endl;
            
            Compiler compiler;
            
            // Test accessing a global variable
            VariableExpr varExpr("globalVar");
            int reg = compiler.compileExpr(&varExpr);
            
            // Should allocate a new register
            assert(reg >= 0);
            
            // Should generate GETGLOBAL instruction
            assert(compiler.getCodeSize() == 1);
            
            // Verify the constant was added for the variable name
            assert(compiler.getConstantCount() == 1);
            
            std::cout << "[OK] Global variable access test passed" << std::endl;
        }
        
        void CompilerVariableTest::testVariableResolution() {
            std::cout << "Testing variable resolution priority..." << std::endl;
            
            Compiler compiler;
            
            // Add a global variable access first
            VariableExpr globalExpr("testVar");
            int globalReg = compiler.compileExpr(&globalExpr);
            assert(compiler.getCodeSize() == 1); // GETGLOBAL instruction
            
            // Now add a local variable with the same name
            compiler.beginScope();
            int localSlot = compiler.allocReg();
            compiler.addLocal("testVar", localSlot);
            
            // Access the variable again - should resolve to local
            VariableExpr localExpr("testVar");
            int localReg = compiler.compileExpr(&localExpr);
            
            // Should return the local slot
            assert(localReg == localSlot);
            
            // No new instruction should be generated
            assert(compiler.getCodeSize() == 1);
            
            compiler.endScope();
            
            std::cout << "[OK] Variable resolution test passed" << std::endl;
        }
        
        void CompilerVariableTest::testScopeHandling() {
            std::cout << "Testing scope handling..." << std::endl;
            
            Compiler compiler;
            
            // Outer scope
            compiler.beginScope();
            int outerSlot = compiler.allocReg();
            compiler.addLocal("x", outerSlot);
            
            // Inner scope with shadowing variable
            compiler.beginScope();
            int innerSlot = compiler.allocReg();
            compiler.addLocal("x", innerSlot);
            
            // Should resolve to inner scope variable
            VariableExpr innerExpr("x");
            int innerReg = compiler.compileExpr(&innerExpr);
            assert(innerReg == innerSlot);
            
            compiler.endScope(); // Exit inner scope
            
            // Should now resolve to outer scope variable
            VariableExpr outerExpr("x");
            int outerReg = compiler.compileExpr(&outerExpr);
            assert(outerReg == outerSlot);
            
            compiler.endScope(); // Exit outer scope
            
            std::cout << "[OK] Scope handling test passed" << std::endl;
        }
        
        void CompilerVariableTest::testRegisterAllocation() {
            std::cout << "Testing register allocation for variables..." << std::endl;
            
            Compiler compiler;
            
            // Test multiple global variable accesses
            VariableExpr var1("global1");
            VariableExpr var2("global2");
            VariableExpr var3("global3");
            
            int reg1 = compiler.compileExpr(&var1);
            int reg2 = compiler.compileExpr(&var2);
            int reg3 = compiler.compileExpr(&var3);
            
            // Should allocate different registers
            assert(reg1 != reg2);
            assert(reg2 != reg3);
            assert(reg1 != reg3);
            
            // Should generate three GETGLOBAL instructions
            assert(compiler.getCodeSize() == 3);
            
            std::cout << "[OK] Register allocation test passed" << std::endl;
        }
        
        void CompilerVariableTest::testInstructionGeneration() {
            std::cout << "Testing instruction generation for variables..." << std::endl;
            
            Compiler compiler;
            
            // Test global variable instruction
            VariableExpr globalVar("testGlobal");
            int globalReg = compiler.compileExpr(&globalVar);
            
            // Verify instruction was generated
            assert(compiler.getCodeSize() == 1);
            
            // Test local variable (no instruction)
            compiler.beginScope();
            int localSlot = compiler.allocReg();
            compiler.addLocal("testLocal", localSlot);
            
            VariableExpr localVar("testLocal");
            int localReg = compiler.compileExpr(&localVar);
            
            // No new instruction should be generated
            assert(compiler.getCodeSize() == 1);
            
            compiler.endScope();
            
            std::cout << "[OK] Instruction generation test passed" << std::endl;
        }
        
        void CompilerVariableTest::testErrorHandling() {
            std::cout << "Testing error handling for variable compilation..." << std::endl;
            
            Compiler compiler;
            
            // Test null expression handling
            try {
                ExpressionCompiler exprCompiler(&compiler);
                exprCompiler.compileVariable(nullptr);
                assert(false); // Should not reach here
            } catch (const LuaException& e) {
                // Expected exception
                assert(std::string(e.what()).find("Null") != std::string::npos ||
                       std::string(e.what()).find("null") != std::string::npos);
            }
            
            std::cout << "[OK] Error handling test passed" << std::endl;
        }
    }
}