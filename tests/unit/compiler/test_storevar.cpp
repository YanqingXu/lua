/**
 * @file test_storevar.cpp
 * @brief 测试变量存储统一接口（luaK_storevar）
 * 
 * 测试 luaK_storevar 函数以及各种类型变量的赋值
 * 验证第一阶段第三项的实现：变量存储统一接口
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>

using namespace Lua;
using namespace LuaTest;

/**
 * @brief 测试局部变量赋值
 * 代码: local x = 10
 */
void testLocalVarAssignment(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    
    const char* code = "local x = 10";
    Parser parser(code);
    Chunk chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);
    
    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");
    
    // 局部变量赋值应该生成 LOADK 指令（将常量加载到寄存器）
    bool hasLoadK = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::LOADK) {
            hasLoadK = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasLoadK, "Generated LOADK instruction");
    
    delete proto;
}

/**
 * @brief 测试全局变量赋值
 * 代码: g = 20
 */
void testGlobalVarAssignment(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    
    const char* code = "g = 20";
    Parser parser(code);
    Chunk chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);
    
    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    
    // 全局变量赋值应该生成 SETGLOBAL 指令
    bool hasSetGlobal = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SETGLOBAL) {
            hasSetGlobal = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasSetGlobal, "Generated SETGLOBAL instruction");
    
    delete proto;
}

/**
 * @brief 测试表索引赋值（字符串键）
 * 代码: t["key"] = 30
 */
void testTableIndexAssignment(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    
    const char* code = "t[\"key\"] = 30";
    Parser parser(code);
    Chunk chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);
    
    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    
    // 表索引赋值应该生成 SETTABLE 指令
    bool hasSetTable = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SETTABLE) {
            hasSetTable = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasSetTable, "Generated SETTABLE instruction");
    
    delete proto;
}

/**
 * @brief 测试表成员赋值
 * 代码: t.field = 40
 */
void testTableMemberAssignment(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    
    const char* code = "t.field = 40";
    Parser parser(code);
    Chunk chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);
    
    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    
    // 表成员赋值应该生成 SETTABLE 指令
    bool hasSetTable = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SETTABLE) {
            hasSetTable = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasSetTable, "Generated SETTABLE instruction");
    
    delete proto;
}

/**
 * @brief 测试多重赋值
 * 代码: a, b = 1, 2
 */
void testMultipleAssignment(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    
    const char* code = "a, b = 1, 2";
    Parser parser(code);
    Chunk chunk = parser.parse();
    
    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);
    
    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    
    // 多重赋值应该生成两个 SETGLOBAL 指令
    int setGlobalCount = 0;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SETGLOBAL) {
            setGlobalCount++;
        }
    }
    ASSERT_TRUE(suite, setGlobalCount >= 2, "Generated two SETGLOBAL instructions");
    
    delete proto;
}

/**
 * @brief 注册所有变量存储测试
 */
void registerStorevarTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest("Variable Storage", "Local Variable Assignment", testLocalVarAssignment);
    registry.registerTest("Variable Storage", "Global Variable Assignment", testGlobalVarAssignment);
    registry.registerTest("Variable Storage", "Table Index Assignment", testTableIndexAssignment);
    registry.registerTest("Variable Storage", "Table Member Assignment", testTableMemberAssignment);
    registry.registerTest("Variable Storage", "Multiple Assignment", testMultipleAssignment);
}

