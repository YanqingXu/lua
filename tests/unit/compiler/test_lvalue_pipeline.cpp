/**
 * @file test_lvalue_pipeline.cpp
 * @brief PR-3 LValue Pipeline 单元测试
 *
 * 测试 emitLValue / emitStore 通道：
 * - 局部变量、全局变量、upvalue、t[k]、obj.x 各种左值类型
 * - 多重赋值中混合目标
 * - 嵌套表赋值
 * - obj.x = obj.x + 1 自增模式
 * - a,b = f() 多返回值写入混合左值
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "lib/lib_manager.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

LuaState* createFullState() {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    return L;
}

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
        Proto* proto = codegen.generate(chunk, "test_lvalue_pipeline");
        if (proto == nullptr) return false;

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

// Helper: compile and count occurrences of an opcode
int countOpcode(const char* code, OpCode op) {
    StringPool& pool = StringPool::getInstance();
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);
    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);
    int count = 0;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == op)
            count++;
    }
    delete proto;
    return count;
}

} // namespace

// =====================================================================
// 字节码层面测试
// =====================================================================

void testLValueLocalBytecode(TestSuite& suite) {
    ASSERT_TRUE(suite, countOpcode("local x\n x = 10", OpCode::LOADK) >= 1,
                "Local assignment generates LOADK");
}

void testLValueGlobalBytecode(TestSuite& suite) {
    ASSERT_TRUE(suite, countOpcode("g = 20", OpCode::SETGLOBAL) >= 1,
                "Global assignment generates SETGLOBAL");
}

void testLValueTableIndexBytecode(TestSuite& suite) {
    ASSERT_TRUE(suite, countOpcode("t[\"key\"] = 30", OpCode::SETTABLE) >= 1,
                "Table index assignment generates SETTABLE");
}

void testLValueMemberBytecode(TestSuite& suite) {
    ASSERT_TRUE(suite, countOpcode("t.field = 40", OpCode::SETTABLE) >= 1,
                "Member assignment generates SETTABLE");
}

void testLValueMultiAssignBytecode(TestSuite& suite) {
    ASSERT_TRUE(suite, countOpcode("a, b = 1, 2", OpCode::SETGLOBAL) >= 2,
                "Multi assignment generates multiple SETGLOBAL");
}

// =====================================================================
// 运行时语义测试
// =====================================================================

void testLValueLocalRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local x = 0
        x = 42
        assert(x == 42, "local assignment")
    )lua");
    ASSERT_TRUE(suite, ok, "Local lvalue runtime");
    delete L;
}

void testLValueGlobalRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        g_val = 99
        assert(g_val == 99, "global assignment")
    )lua");
    ASSERT_TRUE(suite, ok, "Global lvalue runtime");
    delete L;
}

void testLValueUpvalueRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local x = 0
        local function set(v)
            x = v
        end
        set(77)
        assert(x == 77, "upvalue write-back")
    )lua");
    ASSERT_TRUE(suite, ok, "Upvalue lvalue runtime");
    delete L;
}

void testLValueTableIndexRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local t = {}
        local key = "abc"
        t[key] = 55
        assert(t.abc == 55, "table index assignment")
        t[1] = 100
        assert(t[1] == 100, "numeric index assignment")
    )lua");
    ASSERT_TRUE(suite, ok, "Table index lvalue runtime");
    delete L;
}

void testLValueMemberRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local t = {}
        t.x = 10
        t.y = 20
        assert(t.x == 10, "member assignment x")
        assert(t.y == 20, "member assignment y")
    )lua");
    ASSERT_TRUE(suite, ok, "Member lvalue runtime");
    delete L;
}

void testLValueNestedTableRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local a = { b = { c = {} } }
        a.b.c.d = 42
        assert(a.b.c.d == 42, "nested member deep write")

        local key = "dynamic"
        a.b[key] = 88
        assert(a.b.dynamic == 88, "nested indexed write")
    )lua");
    ASSERT_TRUE(suite, ok, "Nested table lvalue runtime");
    delete L;
}

void testLValueSelfIncrementRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local obj = { x = 10 }
        obj.x = obj.x + 1
        assert(obj.x == 11, "obj.x = obj.x + 1")
    )lua");
    ASSERT_TRUE(suite, ok, "Self increment lvalue runtime");
    delete L;
}

void testLValueMixedMultiAssignRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        g_val = 0
        local t = {}
        local loc

        loc, g_val, t.answer = 10, 20, 30

        assert(loc == 10, "mixed multi-assign local")
        assert(g_val == 20, "mixed multi-assign global")
        assert(t.answer == 30, "mixed multi-assign member")
    )lua");
    ASSERT_TRUE(suite, ok, "Mixed multi-assign lvalue runtime");
    delete L;
}

void testLValueMultiRetTargetsRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function triple()
            return 7, 8, 9
        end

        g_slot = 0
        local holder = {}
        local local_slot
        local_slot, g_slot, holder.answer = triple()

        assert(local_slot == 7, "multiret local target")
        assert(g_slot == 8, "multiret global target")
        assert(holder.answer == 9, "multiret member target")
    )lua");
    ASSERT_TRUE(suite, ok, "Multi-return mixed lvalue runtime");
    delete L;
}

void testLValueExcessVarsNilRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local a, b, c = 1
        assert(a == 1, "first gets value")
        assert(b == nil, "second is nil")
        assert(c == nil, "third is nil")

        g1, g2, g3 = 10
        assert(g1 == 10, "first global gets value")
        assert(g2 == nil, "second global is nil")
        assert(g3 == nil, "third global is nil")
    )lua");
    ASSERT_TRUE(suite, ok, "Excess vars get nil runtime");
    delete L;
}

void testLValueTableMultiIndexRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local t = {}
        t[1], t[2], t[3] = 10, 20, 30
        assert(t[1] == 10, "t[1]")
        assert(t[2] == 20, "t[2]")
        assert(t[3] == 30, "t[3]")
    )lua");
    ASSERT_TRUE(suite, ok, "Table multi-index lvalue runtime");
    delete L;
}

void testLValueMixedTableAndGlobalMultiReturn(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function pair() return "a", "b" end
        local t = {}
        t.x, g_y = pair()
        assert(t.x == "a", "table gets first return")
        assert(g_y == "b", "global gets second return")
    )lua");
    ASSERT_TRUE(suite, ok, "Mixed table+global multi-return runtime");
    delete L;
}

// =====================================================================
// 注册
// =====================================================================

void registerLValuePipelineTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("LValue Pipeline", "Local Bytecode", testLValueLocalBytecode);
    registry.registerTest("LValue Pipeline", "Global Bytecode", testLValueGlobalBytecode);
    registry.registerTest("LValue Pipeline", "Table Index Bytecode", testLValueTableIndexBytecode);
    registry.registerTest("LValue Pipeline", "Member Bytecode", testLValueMemberBytecode);
    registry.registerTest("LValue Pipeline", "Multi Assign Bytecode", testLValueMultiAssignBytecode);
    registry.registerTest("LValue Pipeline", "Local Runtime", testLValueLocalRuntime);
    registry.registerTest("LValue Pipeline", "Global Runtime", testLValueGlobalRuntime);
    registry.registerTest("LValue Pipeline", "Upvalue Runtime", testLValueUpvalueRuntime);
    registry.registerTest("LValue Pipeline", "Table Index Runtime", testLValueTableIndexRuntime);
    registry.registerTest("LValue Pipeline", "Member Runtime", testLValueMemberRuntime);
    registry.registerTest("LValue Pipeline", "Nested Table Runtime", testLValueNestedTableRuntime);
    registry.registerTest("LValue Pipeline", "Self Increment Runtime", testLValueSelfIncrementRuntime);
    registry.registerTest("LValue Pipeline", "Mixed Multi-Assign Runtime", testLValueMixedMultiAssignRuntime);
    registry.registerTest("LValue Pipeline", "Multi-Return Targets Runtime", testLValueMultiRetTargetsRuntime);
    registry.registerTest("LValue Pipeline", "Excess Vars Nil Runtime", testLValueExcessVarsNilRuntime);
    registry.registerTest("LValue Pipeline", "Table Multi-Index Runtime", testLValueTableMultiIndexRuntime);
    registry.registerTest("LValue Pipeline", "Mixed Table+Global MultiRet", testLValueMixedTableAndGlobalMultiReturn);
}
