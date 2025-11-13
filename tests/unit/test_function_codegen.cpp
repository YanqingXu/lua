/**
 * @file test_function_codegen.cpp
 * @brief 测试函数定义和调用的代码生成
 */

#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>
#include <cassert>

using namespace Lua;

void testSimpleFunctionDef() {
    std::cout << "[TEST 1] Simple Function Definition..." << std::endl;
    
    StringPool pool;
    
    // 测试: function add(a, b) return a + b end
    const char* code = "function add(a, b) return a + b end";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    // 验证生成的字节码
    assert(proto != nullptr);
    assert(proto->getInstructionCount() > 0);
    
    // 应该有一个子函数
    assert(proto->getSubProtoCount() == 1);
    
    // 检查子函数
    Proto* subProto = proto->getSubProto(0);
    assert(subProto != nullptr);
    assert(subProto->getNumParams() == 2);  // 两个参数
    
    std::cout << "  Main proto: " << proto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  Sub proto: " << subProto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  Sub proto params: " << (int)subProto->getNumParams() << std::endl;
    std::cout << "  PASS" << std::endl;
}

void testLocalFunctionDef() {
    std::cout << "[TEST 2] Local Function Definition..." << std::endl;
    
    StringPool pool;
    
    // 测试: local function foo() end
    const char* code = "local function foo() end";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    assert(proto != nullptr);
    assert(proto->getSubProtoCount() == 1);
    
    std::cout << "  Generated " << proto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  PASS" << std::endl;
}

void testFunctionExpr() {
    std::cout << "[TEST 3] Function Expression..." << std::endl;
    
    StringPool pool;
    
    // 测试: local f = function(x) return x * 2 end
    const char* code = "local f = function(x) return x * 2 end";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    assert(proto != nullptr);
    assert(proto->getSubProtoCount() == 1);
    
    Proto* subProto = proto->getSubProto(0);
    assert(subProto->getNumParams() == 1);  // 一个参数
    
    std::cout << "  Generated " << proto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  PASS" << std::endl;
}

void testFunctionCall() {
    std::cout << "[TEST 4] Function Call..." << std::endl;
    
    StringPool pool;
    
    // 测试: local result = add(1, 2)
    const char* code = "local result = add(1, 2)";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    assert(proto != nullptr);
    assert(proto->getInstructionCount() > 0);
    
    // 检查是否有CALL指令
    bool hasCall = false;
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::CALL) {
            hasCall = true;
            std::cout << "  Found CALL instruction at pc=" << i << std::endl;
            break;
        }
    }
    assert(hasCall);
    
    std::cout << "  Generated " << proto->getInstructionCount() << " instructions" << std::endl;
    std::cout << "  PASS" << std::endl;
}

void testVarargFunction() {
    std::cout << "[TEST 5] Vararg Function..." << std::endl;
    
    StringPool pool;
    
    // 测试: function foo(...) end
    const char* code = "function foo(...) end";
    Lexer lexer(code);
    Parser parser(&lexer, &pool);
    auto chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    auto proto = codegen.generate(chunk.get());
    
    assert(proto != nullptr);
    assert(proto->getSubProtoCount() == 1);
    
    Proto* subProto = proto->getSubProto(0);
    assert(subProto->isVararg());  // 应该是可变参数函数
    
    std::cout << "  Vararg: " << (subProto->isVararg() ? "yes" : "no") << std::endl;
    std::cout << "  PASS" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Function Definition and Call Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testSimpleFunctionDef();
        testLocalFunctionDef();
        testFunctionExpr();
        testFunctionCall();
        testVarargFunction();
        
        std::cout << "\n[SUCCESS] All tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Test failed: " << e.what() << std::endl;
        return 1;
    }
}

