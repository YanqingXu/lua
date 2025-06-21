#include "compiler_literal_test.hpp"
#include "../../compiler/expression_compiler.hpp"
#include "../../compiler/compiler.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../parser/ast/expressions.hpp"
#include <cassert>

namespace Lua::Tests {

void CompilerLiteralTest::testBasicLiterals() {
    RUN_TEST(CompilerLiteralTest, testNilLiteral);
    RUN_TEST(CompilerLiteralTest, testBooleanLiterals);
    RUN_TEST(CompilerLiteralTest, testNumberLiterals);
    RUN_TEST(CompilerLiteralTest, testStringLiterals);
    RUN_TEST(CompilerLiteralTest, testComplexLiterals);
}

void CompilerLiteralTest::testConstantManagement() {
    RUN_TEST(CompilerLiteralTest, testLiteralConstantTable);
    RUN_TEST(CompilerLiteralTest, testRegisterAllocation);
}

void CompilerLiteralTest::testInstructionGeneration() {
    TestUtils::printInfo("Testing instruction generation for literals");
    
    Compiler compiler;
    
    // Test nil literal instruction
    Value nilValue;
    LiteralExpr nilExpr(nilValue);
    int reg1 = compiler.compileExpr(&nilExpr);
    
    // Verify LOADNIL instruction was generated
    TestUtils::printTestResult("LOADNIL instruction generated", compiler.getCodeSize() == 1);
    
    // Test boolean literal instruction
    Value boolValue(true);
    LiteralExpr boolExpr(boolValue);
    int reg2 = compiler.compileExpr(&boolExpr);
    
    // Verify LOADBOOL instruction was generated
    TestUtils::printTestResult("LOADBOOL instruction generated", compiler.getCodeSize() == 2);
    
    // Test number literal instruction
    Value numValue(42.5);
    LiteralExpr numExpr(numValue);
    int reg3 = compiler.compileExpr(&numExpr);
    
    // Verify LOADK instruction was generated
    TestUtils::printTestResult("Number LOADK instruction generated", compiler.getCodeSize() == 3);
    
    // Test string literal instruction
    Value strValue("hello");
    LiteralExpr strExpr(strValue);
    int reg4 = compiler.compileExpr(&strExpr);
    
    // Verify LOADK instruction was generated
    TestUtils::printTestResult("String LOADK instruction generated", compiler.getCodeSize() == 4);
}

void CompilerLiteralTest::testErrorHandling() {
    TestUtils::printInfo("Testing error handling for literal compilation");
    
    Compiler compiler;
    
    // Test null expression handling
    try {
        compiler.compileExpr(nullptr);
        TestUtils::printTestResult("Null expression throws exception", false);
    } catch (const LuaException& e) {
        TestUtils::printTestResult("Null expression throws exception", true);
    }
}
        
void CompilerLiteralTest::testNilLiteral() {
    TestUtils::printInfo("Testing nil literal compilation");
    
    Compiler compiler;
    Value nilValue;
    LiteralExpr nilExpr(nilValue);
    
    int reg = compiler.compileExpr(&nilExpr);
    
    // Verify register allocation
    TestUtils::printTestResult("Nil literal register allocation", reg >= 0);
    
    // Verify instruction generation
    TestUtils::printTestResult("Nil literal instruction generation", compiler.getCodeSize() == 1);
}
        
void CompilerLiteralTest::testBooleanLiterals() {
    TestUtils::printInfo("Testing boolean literal compilation");
    
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
    TestUtils::printTestResult("True literal register allocation", reg1 >= 0);
    TestUtils::printTestResult("False literal register allocation", reg2 >= 0);
    TestUtils::printTestResult("Different registers for boolean literals", reg1 != reg2);
    
    // Verify instruction generation
    TestUtils::printTestResult("Boolean literals instruction generation", compiler.getCodeSize() == 2);
}
        
void CompilerLiteralTest::testNumberLiterals() {
    TestUtils::printInfo("Testing number literal compilation");
    
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
    TestUtils::printTestResult("Integer literal register allocation", reg1 >= 0);
    TestUtils::printTestResult("Float literal register allocation", reg2 >= 0);
    TestUtils::printTestResult("Negative number literal register allocation", reg3 >= 0);
    TestUtils::printTestResult("Zero literal register allocation", reg4 >= 0);
    
    // Verify instruction generation
    TestUtils::printTestResult("Number literals instruction generation", compiler.getCodeSize() == 4);
}
        
void CompilerLiteralTest::testStringLiterals() {
    TestUtils::printInfo("Testing string literal compilation");
    
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
    TestUtils::printTestResult("Simple string register allocation", reg1 >= 0);
    TestUtils::printTestResult("Empty string register allocation", reg2 >= 0);
    TestUtils::printTestResult("Special chars string register allocation", reg3 >= 0);
    TestUtils::printTestResult("Long string register allocation", reg4 >= 0);
    
    // Verify instruction generation
    TestUtils::printTestResult("String literals instruction generation", compiler.getCodeSize() == 4);
}
        
void CompilerLiteralTest::testComplexLiterals() {
    TestUtils::printInfo("Testing complex literal compilation");
    
    Compiler compiler;
    
    // Test table literal (empty table)
    auto table = make_gc_table();
    Value tableValue(table);
    LiteralExpr tableExpr(tableValue);
    int reg1 = compiler.compileExpr(&tableExpr);
    
    // Verify register allocation
    TestUtils::printTestResult("Table literal register allocation", reg1 >= 0);
    
    // Verify instruction generation
    TestUtils::printTestResult("Table literal instruction generation", compiler.getCodeSize() == 1);
}
        
void CompilerLiteralTest::testLiteralConstantTable() {
    TestUtils::printInfo("Testing literal constant table management");
    
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
    TestUtils::printTestResult("Constant table instruction generation", compiler.getCodeSize() == 6);
}
        
void CompilerLiteralTest::testRegisterAllocation() {
    TestUtils::printInfo("Testing register allocation for literals");
    
    Compiler compiler;
    
    // Compile multiple literals and verify register allocation
    Vec<int> registers;
    
    for (int i = 0; i < 10; ++i) {
        Value value(i);
        LiteralExpr expr(value);
        int reg = compiler.compileExpr(&expr);
        registers.push_back(reg);
        TestUtils::printTestResult("Register allocation for literal " + std::to_string(i), reg >= 0);
    }
    
    // Verify all registers are different (assuming no register reuse optimization)
    bool allDifferent = true;
    for (size_t i = 0; i < registers.size() && allDifferent; ++i) {
        for (size_t j = i + 1; j < registers.size(); ++j) {
            if (registers[i] == registers[j]) {
                allDifferent = false;
                break;
            }
        }
    }
    
    TestUtils::printTestResult("All registers are different", allDifferent);
}
        
        void CompilerLiteralTest::testInstructionGeneration() {
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
        
        void CompilerLiteralTest::testErrorHandling() {
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
            
}

} // namespace Lua::Tests