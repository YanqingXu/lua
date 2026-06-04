/**
 * @file test_indexed_access.cpp
 * @brief 测试表索引访问和成员访问的代码生成
 * 
 * 测试 luaK_indexed 函数以及 IndexExpr 和 MemberExpr 的处理
 * 验证第一阶段第一项的实现：表索引访问支持
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer/lexer.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>

using namespace Lua;
using namespace LuaTest;

/**
 * @brief 辅助函数：打印指令信息（用于调试）
 */
void printInstructions(Proto* proto) {
    std::cout << "  Generated " << proto->getInstructionCount() << " instructions:" << std::endl;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        OpCode op = GET_OPCODE(inst);
        std::cout << "    [" << i << "] " << getOpName(op);
        
        switch (op) {
            case OpCode::GETTABLE:
                std::cout << " A=" << GETARG_A(inst) 
                         << " B=" << GETARG_B(inst) 
                         << " C=" << GETARG_C(inst);
                break;
            case OpCode::GETGLOBAL:
                std::cout << " A=" << GETARG_A(inst) 
                         << " Bx=" << GETARG_Bx(inst);
                break;
            case OpCode::LOADK:
                std::cout << " A=" << GETARG_A(inst) 
                         << " Bx=" << GETARG_Bx(inst);
                break;
            case OpCode::MOVE:
                std::cout << " A=" << GETARG_A(inst) 
                         << " B=" << GETARG_B(inst);
                break;
            default:
                break;
        }
        std::cout << std::endl;
    }
}

/**
 * @brief 测试简单的表索引访问（字符串键）
 * 代码: local x = t["key"]
 * 注意：假设 t 是全局变量
 */
void testSimpleIndexAccessStringKey(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local x = t[\"key\"]";
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

    // 验证生成了 GETTABLE 指令
    bool hasGetTable = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::GETTABLE) {
            hasGetTable = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasGetTable, "Generated GETTABLE instruction");

}

/**
 * @brief 测试成员访问（应转换为表索引）
 * 代码: local x = t.member
 * 注意：假设 t 是全局变量
 */
void testMemberAccess(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local x = t.member";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 成员访问应该生成 GETTABLE 指令
    bool hasGetTable = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::GETTABLE) {
            hasGetTable = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasGetTable, "Member access generates GETTABLE");

}

/**
 * @brief 测试数字索引访问
 * 代码: local x = t[1]
 * 注意：假设 t 是全局变量
 */
void testIndexAccessNumberKey(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local x = t[1]";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    bool hasGetTable = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::GETTABLE) {
            hasGetTable = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasGetTable, "Number index generates GETTABLE");

}

/**
 * @brief 测试嵌套表访问
 * 代码: local x = t.a.b
 * 注意：假设 t 是全局变量
 */
void testNestedMemberAccess(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local x = t.a.b";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 嵌套访问应该生成多个 GETTABLE 指令
    int getTableCount = 0;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::GETTABLE) {
            getTableCount++;
        }
    }
    ASSERT_TRUE(suite, getTableCount >= 2, "Nested access generates multiple GETTABLE");

}

/**
 * @brief 测试全局变量访问（应生成 GETGLOBAL）
 * 代码: local x = print
 */
void testGlobalVariableAccess(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local x = print";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 全局变量应该生成 GETGLOBAL 指令
    bool hasGetGlobal = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::GETGLOBAL) {
            hasGetGlobal = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasGetGlobal, "Global variable generates GETGLOBAL");

}

/**
 * @brief 测试动态索引（变量作为键）
 * 代码: local x = t[key]
 * 注意：假设 t 和 key 都是全局变量
 */
void testDynamicIndex(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local x = t[key]";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    bool hasGetTable = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::GETTABLE) {
            hasGetTable = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasGetTable, "Dynamic index generates GETTABLE");

}

/**
 * @brief 测试混合访问（索引和成员）
 * 代码: local x = t["a"].b[1]
 * 注意：假设 t 是全局变量
 */
void testMixedAccess(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local x = t[\"a\"].b[1]";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 应该生成多个 GETTABLE 指令
    int getTableCount = 0;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::GETTABLE) {
            getTableCount++;
        }
    }
    ASSERT_TRUE(suite, getTableCount >= 3, "Mixed access generates multiple GETTABLE");

}

/**
 * @brief 测试局部变量表访问
 * 代码: local x = t.a
 * 注意：假设 t 是全局变量，测试局部变量接收结果
 */
void testLocalTableAccess(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local x = t.a";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 应该有 GETGLOBAL 和 GETTABLE 指令
    bool hasGetGlobal = false;
    bool hasGetTable = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        OpCode op = GET_OPCODE(proto->getInstruction(i));
        if (op == OpCode::GETGLOBAL) hasGetGlobal = true;
        if (op == OpCode::GETTABLE) hasGetTable = true;
    }
    ASSERT_TRUE(suite, hasGetGlobal, "Global variable generates GETGLOBAL");
    ASSERT_TRUE(suite, hasGetTable, "Table access generates GETTABLE");

}

/**
 * @brief 注册所有表索引访问测试
 */
void registerIndexedAccessTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Table Indexed Access", "Simple Index (String Key)", testSimpleIndexAccessStringKey);
    registry.registerTest("Table Indexed Access", "Member Access", testMemberAccess);
    registry.registerTest("Table Indexed Access", "Index (Number Key)", testIndexAccessNumberKey);
    registry.registerTest("Table Indexed Access", "Nested Member Access", testNestedMemberAccess);
    registry.registerTest("Table Indexed Access", "Global Variable Access", testGlobalVariableAccess);
    registry.registerTest("Table Indexed Access", "Dynamic Index", testDynamicIndex);
    registry.registerTest("Table Indexed Access", "Mixed Access", testMixedAccess);
    registry.registerTest("Table Indexed Access", "Local Table Access", testLocalTableAccess);
}


