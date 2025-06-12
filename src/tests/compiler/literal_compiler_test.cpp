#include "literal_compiler_test.hpp"
#include "../../compiler/expression_compiler.hpp"
#include "../../compiler/compiler.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../parser/ast/expressions.hpp"
#include <cassert>
#include <iostream>

namespace Lua {
    namespace Tests {
        
        void LiteralCompilerTest::runAllTests() {
            std::cout << "Running Literal Compiler Tests..." << std::endl;
            
            testNilLiteral();
            testBooleanLiterals();
            testNumberLiterals();
            testStringLiterals();
            testComplexLiterals();
            testLiteralConstantTable();
            testRegisterAllocation();
            testInstructionGeneration();
            testErrorHandling();
            
            std::cout << "All Literal Compiler Tests Passed!" << std::endl;
        }
        
        void LiteralCompilerTest::testNilLiteral() {
            std::cout << "Testing nil literal compilation..." << std::endl;
            
            Compiler compiler;
            Value nilValue;
            LiteralExpr nilExpr(nilValue);
            
            int reg = compiler.compileExpr(&nilExpr);
            
            // Verify register allocation
            assert(reg >= 0);
            
            // Verify instruction generation
            assert(compiler.getCodeSize() == 1);
            
            std::cout << "[OK] Nil literal compilation test passed" << std::endl;
        }
        
        void LiteralCompilerTest::testBooleanLiterals() {
            std::cout << "Testing boolean literal compilation..." << std::endl;
            
            Compiler compiler;
            
            // Test true literal
            Value trueValue(true);
            LiteralExpr trueExpr(trueValue);
            int reg1 = compiler.compileExpr(&trueExpr);
            
            // Test false literal
            Value falseValue(false);
            LiteralExpr falseExpr(falseValue);
            int reg2 = compiler.compileExpr(&falseExpr);
            
            // Verify register allocation
            assert(reg1 >= 0);
            assert(reg2 >= 0);
            assert(reg1 != reg2); // Different registers
            
            // Verify instruction generation
            assert(compiler.getCodeSize() == 2);
            
            std::cout << "[OK] Boolean literal compilation test passed" << std::endl;
        }
        
        void LiteralCompilerTest::testNumberLiterals() {
            std::cout << "Testing number literal compilation..." << std::endl;
            
            Compiler compiler;
            
            // Test integer
            Value intValue(42);
            LiteralExpr intExpr(intValue);
            int reg1 = compiler.compileExpr(&intExpr);
            
            // Test floating point
            Value floatValue(3.14);
            LiteralExpr floatExpr(floatValue);
            int reg2 = compiler.compileExpr(&floatExpr);
            
            // Test negative number
            Value negValue(-123.456);
            LiteralExpr negExpr(negValue);
            int reg3 = compiler.compileExpr(&negExpr);
            
            // Test zero
            Value zeroValue(0.0);
            LiteralExpr zeroExpr(zeroValue);
            int reg4 = compiler.compileExpr(&zeroExpr);
            
            // Verify register allocation
            assert(reg1 >= 0);
            assert(reg2 >= 0);
            assert(reg3 >= 0);
            assert(reg4 >= 0);
            
            // Verify instruction generation
            assert(compiler.getCodeSize() == 4);
            
            std::cout << "[OK] Number literal compilation test passed" << std::endl;
        }
        
        void LiteralCompilerTest::testStringLiterals() {
            std::cout << "Testing string literal compilation..." << std::endl;
            
            Compiler compiler;
            
            // Test simple string
            Value strValue("hello");
            LiteralExpr strExpr(strValue);
            int reg1 = compiler.compileExpr(&strExpr);
            
            // Test empty string
            Value emptyValue("");
            LiteralExpr emptyExpr(emptyValue);
            int reg2 = compiler.compileExpr(&emptyExpr);
            
            // Test string with special characters
            Value specialValue("hello\nworld\t!");
            LiteralExpr specialExpr(specialValue);
            int reg3 = compiler.compileExpr(&specialExpr);
            
            // Test long string
            Value longValue("This is a very long string that tests the string literal compilation functionality");
            LiteralExpr longExpr(longValue);
            int reg4 = compiler.compileExpr(&longExpr);
            
            // Verify register allocation
            assert(reg1 >= 0);
            assert(reg2 >= 0);
            assert(reg3 >= 0);
            assert(reg4 >= 0);
            
            // Verify instruction generation
            assert(compiler.getCodeSize() == 4);
            
            std::cout << "[OK] String literal compilation test passed" << std::endl;
        }
        
        void LiteralCompilerTest::testComplexLiterals() {
            std::cout << "Testing complex literal compilation..." << std::endl;
            
            Compiler compiler;
            
            // Test table literal (empty table)
            auto table = make_gc_table();
            Value tableValue(table);
            LiteralExpr tableExpr(tableValue);
            int reg1 = compiler.compileExpr(&tableExpr);
            
            // Verify register allocation
            assert(reg1 >= 0);
            
            // Verify instruction generation
            assert(compiler.getCodeSize() == 1);
            
            std::cout << "[OK] Complex literal compilation test passed" << std::endl;
        }
        
        void LiteralCompilerTest::testLiteralConstantTable() {
            std::cout << "Testing literal constant table management..." << std::endl;
            
            Compiler compiler;
            
            // Add same number literal multiple times
            Value num1(42);
            Value num2(42); // Same value
            Value num3(43); // Different value
            
            LiteralExpr expr1(num1);
            LiteralExpr expr2(num2);
            LiteralExpr expr3(num3);
            
            compiler.compileExpr(&expr1);
            compiler.compileExpr(&expr2);
            compiler.compileExpr(&expr3);
            
            // Add same string literal multiple times
            Value str1("test");
            Value str2("test"); // Same value
            Value str3("different"); // Different value
            
            LiteralExpr strExpr1(str1);
            LiteralExpr strExpr2(str2);
            LiteralExpr strExpr3(str3);
            
            compiler.compileExpr(&strExpr1);
            compiler.compileExpr(&strExpr2);
            compiler.compileExpr(&strExpr3);
            
            // Verify instruction generation
            assert(compiler.getCodeSize() == 6);
            
            std::cout << "[OK] Literal constant table test passed" << std::endl;
        }
        
        void LiteralCompilerTest::testRegisterAllocation() {
            std::cout << "Testing register allocation for literals..." << std::endl;
            
            Compiler compiler;
            
            // Compile multiple literals and verify register allocation
            Vec<int> registers;
            
            for (int i = 0; i < 10; ++i) {
                Value value(i);
                LiteralExpr expr(value);
                int reg = compiler.compileExpr(&expr);
                registers.push_back(reg);
                assert(reg >= 0);
            }
            
            // Verify all registers are different (assuming no register reuse optimization)
            for (size_t i = 0; i < registers.size(); ++i) {
                for (size_t j = i + 1; j < registers.size(); ++j) {
                    assert(registers[i] != registers[j]);
                }
            }
            
            std::cout << "[OK] Register allocation test passed" << std::endl;
        }
        
        void LiteralCompilerTest::testInstructionGeneration() {
            std::cout << "Testing instruction generation for literals..." << std::endl;
            
            Compiler compiler;
            
            // Test nil literal instruction
            Value nilValue;
            LiteralExpr nilExpr(nilValue);
            int reg1 = compiler.compileExpr(&nilExpr);
            
            // Verify LOADNIL instruction was generated
            assert(compiler.getCodeSize() == 1);
            
            // Test boolean literal instruction
            Value boolValue(true);
            LiteralExpr boolExpr(boolValue);
            int reg2 = compiler.compileExpr(&boolExpr);
            
            // Verify LOADBOOL instruction was generated
            assert(compiler.getCodeSize() == 2);
            
            // Test number literal instruction
            Value numValue(42.5);
            LiteralExpr numExpr(numValue);
            int reg3 = compiler.compileExpr(&numExpr);
            
            // Verify LOADK instruction was generated
            assert(compiler.getCodeSize() == 3);
            
            // Test string literal instruction
            Value strValue("hello");
            LiteralExpr strExpr(strValue);
            int reg4 = compiler.compileExpr(&strExpr);
            
            // Verify LOADK instruction was generated
            assert(compiler.getCodeSize() == 4);
            
            std::cout << "[OK] Instruction generation test passed" << std::endl;
        }
        
        void LiteralCompilerTest::testErrorHandling() {
            std::cout << "Testing error handling for literal compilation..." << std::endl;
            
            Compiler compiler;
            
            // Test null expression handling
            try {
                compiler.compileExpr(nullptr);
                assert(false); // Should throw exception
            } catch (const LuaException& e) {
                // Expected exception
				std::cout << "[OK] Caught expected exception: " << e.what() << std::endl;
            }
            
            std::cout << "[OK] Error handling test passed" << std::endl;
        }
    }
}