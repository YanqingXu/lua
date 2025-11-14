/**
 * @file test_function_codegen.cpp
 * @brief 测试函数定义和调用的代码生成
 */

#include "test_framework.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>
#include <cassert>

using namespace Lua;
using namespace LuaTest;

void testSimpleFunctionDef(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: function add(a, b) return a + b end
    const char* code = "function add(a, b) return a + b end";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    // 验证生成的字节码
    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    // 应该有一个子函数
    ASSERT_EQ(suite, proto->getSubProtoCount(), 1, "Has one sub-function");

    // 检查子函数
    Proto* subProto = proto->getSubProto(0);
    ASSERT_TRUE(suite, subProto != nullptr, "Sub-proto exists");
    ASSERT_EQ(suite, (int)subProto->getNumParams(), 2, "Sub-proto has 2 params");

    delete proto;
}

void testLocalFunctionDef(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: local function foo() end
    const char* code = "local function foo() end";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_EQ(suite, proto->getSubProtoCount(), 1, "Has one sub-function");

    delete proto;
}

void testFunctionExpr(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: local f = function(x) return x * 2 end
    const char* code = "local f = function(x) return x * 2 end";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_EQ(suite, proto->getSubProtoCount(), 1, "Has one sub-function");

    Proto* subProto = proto->getSubProto(0);
    ASSERT_EQ(suite, (int)subProto->getNumParams(), 1, "Sub-proto has 1 param");

    delete proto;
}

void testFunctionCall(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: local result = add(1, 2)
    const char* code = "local result = add(1, 2)";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    // 检查是否有CALL指令
    bool hasCall = false;
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::CALL) {
            hasCall = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasCall, "Has CALL instruction");

    delete proto;
}

void testVarargFunction(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: function foo(...) end
    const char* code = "function foo(...) end";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_EQ(suite, proto->getSubProtoCount(), 1, "Has one sub-function");

    Proto* subProto = proto->getSubProto(0);
    ASSERT_TRUE(suite, subProto->isVararg(), "Sub-proto is vararg");

    delete proto;
}

void registerFunctionCodegenTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Function Codegen", "Simple Function Definition", testSimpleFunctionDef);
    registry.registerTest("Function Codegen", "Local Function Definition", testLocalFunctionDef);
    registry.registerTest("Function Codegen", "Function Expression", testFunctionExpr);
    registry.registerTest("Function Codegen", "Function Call", testFunctionCall);
    registry.registerTest("Function Codegen", "Vararg Function", testVarargFunction);
}

