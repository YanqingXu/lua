/**
 * @file test_storevar.cpp
 * @brief 测试变量存储统一接口（luaK_storevar）
 * 
 * 测试 luaK_storevar 函数以及各种类型变量的赋值
 * 验证第一阶段第三项的实现：变量存储统一接口
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer/lexer.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "lib/lib_manager.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include <iostream>

using namespace Lua;
using namespace LuaTest;

namespace {

bool runLua(LuaState* L, const char* code) {
    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, "test_storevar_runtime");
        if (proto == nullptr) {
            return false;
        }

        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        func->setEnv(L->getGlobalTable());
        VM::execute(L, func);
        delete proto;
        return true;
    } catch (...) {
        return false;
    }
}

LuaState* createFullState() {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    return L;
}

} // namespace

/**
 * @brief 测试局部变量赋值
 * 代码: local x = 10
 */
void testLocalVarAssignment(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    
    const char* code = "local x = 10";
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
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);
    
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
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);
    
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
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);
    
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
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);
    
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
 * @brief 测试混合左值赋值的运行时语义
 */
void testMixedLValueAssignmentRuntime(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        g_value = 0
        holder = {}

        local local_value
        local_value, g_value, holder.answer = 10, 20, 30

        assert(local_value == 10, "local target should receive first value")
        assert(g_value == 20, "global target should receive second value")
        assert(holder.answer == 30, "member target should receive third value")

        local key = "dynamic"
        holder[key] = 99
        assert(holder.dynamic == 99, "indexed target should write through dynamic key")
    )lua");

    ASSERT_TRUE(suite, ok, "Mixed lvalue assignment runtime semantics");

    delete L;
}

/**
 * @brief 测试嵌套成员和索引写回的运行时语义
 */
void testNestedTableStoreRuntime(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        holder = { inner = {} }

        holder.inner.answer = 42
        assert(holder.inner.answer == 42, "nested member assignment should write through")

        local key = "deep"
        holder.inner[key] = 64
        assert(holder.inner.deep == 64, "nested indexed assignment should write through")
    )lua");

    ASSERT_TRUE(suite, ok, "Nested table store runtime semantics");

    delete L;
}

/**
 * @brief 测试多返回值写入混合左值列表
 */
void testMultiReturnMixedTargetsRuntime(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local function triple()
            return 7, 8, 9
        end

        g_slot = 0
        holder = {}

        local local_slot
        local_slot, g_slot, holder.answer = triple()

        assert(local_slot == 7, "local mixed target should receive first return value")
        assert(g_slot == 8, "global mixed target should receive second return value")
        assert(holder.answer == 9, "table mixed target should receive third return value")
    )lua");

    ASSERT_TRUE(suite, ok, "Multi-return mixed lvalue runtime semantics");

    delete L;
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
    registry.registerTest("Variable Storage", "Mixed LValue Assignment Runtime", testMixedLValueAssignmentRuntime);
    registry.registerTest("Variable Storage", "Nested Table Store Runtime", testNestedTableStoreRuntime);
    registry.registerTest("Variable Storage", "Multi Return Mixed Targets Runtime", testMultiReturnMixedTargetsRuntime);
}

