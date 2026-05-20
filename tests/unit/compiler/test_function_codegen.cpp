/**
 * @file test_function_codegen.cpp
 * @brief 测试函数定义和调用的代码生成
 */

#include "../framework/test_framework.hpp"
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
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

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
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

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
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

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
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

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
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_EQ(suite, proto->getSubProtoCount(), 1, "Has one sub-function");

    Proto* subProto = proto->getSubProto(0);
    ASSERT_TRUE(suite, subProto->isVararg(), "Sub-proto is vararg");

    delete proto;
}

void testDebugMetadata(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local x = 42\nprint(x)\n";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk, "test_debug_metadata.lua");

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated with debug metadata");
    ASSERT_TRUE(suite, proto->getSource() != nullptr, "Proto source populated");
    if (proto->getSource() != nullptr) {
        ASSERT_TRUE(suite, std::string(proto->getSource()->c_str()) == "test_debug_metadata.lua", "Proto source matches");
    }

    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions for debug metadata");
    ASSERT_TRUE(suite, proto->getLine(0) > 0, "Instruction line info populated");
    ASSERT_TRUE(suite, proto->getLocVarCount() >= 1, "Local variable metadata populated");
    if (proto->getLocVarCount() >= 1) {
        ASSERT_TRUE(suite, std::string(proto->getLocVar(0).varname->c_str()) == "x", "Local variable name recorded");
    }

    delete proto;
}

void testAssignMultiReturnCall(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code =
        "local ok, err\n"
        "ok, err = pcall(function() error('boom') end)\n";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk, "test_assign_multret.lua");

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated for multi-return assignment");

    bool sawPcallCall = false;
    bool sawErrLoadNil = false;
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::CALL && GETARG_A(inst) == 3) {
            sawPcallCall = true;
            ASSERT_EQ(suite, GETARG_C(inst), 3, "Assignment CALL requests two return values");
        }

        if (GET_OPCODE(inst) == OpCode::LOADNIL && GETARG_A(inst) == 1 && GETARG_B(inst) == 1) {
            sawErrLoadNil = true;
        }
    }

    ASSERT_TRUE(suite, sawPcallCall, "Found CALL used by assignment");
    ASSERT_FALSE(suite, sawErrLoadNil, "Multi-return assignment no longer nils second target");

    delete proto;
}

void registerFunctionCodegenTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Function Codegen", "Simple Function Definition", testSimpleFunctionDef);
    registry.registerTest("Function Codegen", "Local Function Definition", testLocalFunctionDef);
    registry.registerTest("Function Codegen", "Function Expression", testFunctionExpr);
    registry.registerTest("Function Codegen", "Function Call", testFunctionCall);
    registry.registerTest("Function Codegen", "Vararg Function", testVarargFunction);
    registry.registerTest("Function Codegen", "Debug Metadata", testDebugMetadata);
    registry.registerTest("Function Codegen", "Assign Multi Return Call", testAssignMultiReturnCall);
}

