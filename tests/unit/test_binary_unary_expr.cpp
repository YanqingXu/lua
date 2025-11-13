/**
 * @file test_binary_unary_expr.cpp
 * @brief 测试二元和一元表达式的代码生成
 */

#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>
#include <cassert>

using namespace Lua;

void testBinaryArithmetic() {
    std::cout << "[TEST] Binary Arithmetic Expressions..." << std::endl;
    
    StringPool pool;
    
    // 测试: local x = 1 + 2
    const char* code = "local x = 1 + 2";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    // 验证生成的字节码
    assert(proto != nullptr);
    assert(proto->getInstructionCount() > 0);
    
    std::cout << "  Generated " << proto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  PASS" << std::endl;
}

void testBinaryComparison() {
    std::cout << "[TEST] Binary Comparison Expressions..." << std::endl;
    
    StringPool pool;
    
    // 测试: local x = 1 < 2
    const char* code = "local x = 1 < 2";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    assert(proto != nullptr);
    assert(proto->getInstructionCount() > 0);
    
    std::cout << "  Generated " << proto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  PASS" << std::endl;
}

void testBinaryLogical() {
    std::cout << "[TEST] Binary Logical Expressions..." << std::endl;
    
    StringPool pool;
    
    // 测试: local x = true and false
    const char* code = "local x = true and false";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    assert(proto != nullptr);
    assert(proto->getInstructionCount() > 0);
    
    std::cout << "  Generated " << proto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  PASS" << std::endl;
}

void testUnaryExpressions() {
    std::cout << "[TEST] Unary Expressions..." << std::endl;
    
    StringPool pool;
    
    // 测试: local x = -42
    const char* code = "local x = -42";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    assert(proto != nullptr);
    assert(proto->getInstructionCount() > 0);
    
    std::cout << "  Generated " << proto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  PASS" << std::endl;
}

void testComplexExpression() {
    std::cout << "[TEST] Complex Expression..." << std::endl;
    
    StringPool pool;
    
    // 测试: local x = (1 + 2) * 3 - 4
    const char* code = "local x = (1 + 2) * 3 - 4";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    assert(proto != nullptr);
    assert(proto->getInstructionCount() > 0);
    
    std::cout << "  Generated " << proto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  PASS" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Binary and Unary Expression Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testBinaryArithmetic();
        testBinaryComparison();
        testBinaryLogical();
        testUnaryExpressions();
        testComplexExpression();
        
        std::cout << "\n[SUCCESS] All tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Test failed: " << e.what() << std::endl;
        return 1;
    }
}

