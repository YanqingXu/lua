/**
 * @file test_symbol_binding.cpp
 * @brief PR-8 Symbol Binding 单元测试
 *
 * 测试 resolve() / symbolToValue() / symbolToLValue() 的统一名字绑定通道：
 * - 局部变量解析（Local）
 * - 全局变量解析（Global）
 * - 上值解析（Upvalue）
 * - 名字遮蔽（shadowing）
 * - emitValue/emitLValue 通过 resolve() 收敛
 * - 函数表路径（table path）加载通过 resolve() 收敛
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/codegen_types.hpp"
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
        Chunk chunk = parser.parse();
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, "test_symbol_binding");
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

int countOpcode(const char* code, OpCode op) {
    StringPool& pool = StringPool::getInstance();
    Parser parser(code);
    Chunk chunk = parser.parse();
    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk, "test_symbol_binding");
    if (proto == nullptr) return 0;

    int count = 0;
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == op) {
            count++;
        }
    }
    delete proto;
    return count;
}

f64 getGlobalNumber(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    return v.asNumber();
}

Str getGlobalString(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    return v.asString()->getData();
}

} // anonymous namespace

// =============================================================================
// SymbolRef 基本结构与转换测试
// =============================================================================

void testSymbolRefDefaultInvalid(TestSuite& suite) {
    SymbolRef sym;
    ASSERT_FALSE(suite, sym.valid(), "default SymbolRef should be invalid");
    ASSERT_TRUE(suite, sym.kind == SymbolRef::Kind::None, "default kind should be None");
    ASSERT_EQ(suite, -1, sym.index, "default index should be -1");
}

void testSymbolToValueLocal(TestSuite& suite) {
    SymbolRef sym;
    sym.kind = SymbolRef::Kind::Local;
    sym.index = 3;
    sym.name = "x";
    StringPool& pool = StringPool::getInstance();
    CodeGenerator codegen(&pool);
    ValueResult vr = codegen.symbolToValue(sym);
    ASSERT_TRUE(suite, vr.kind == ValueResult::Kind::Register, "Local → Register kind");
    ASSERT_TRUE(suite, vr.access == ValueResult::AccessKind::Local, "Local → Local access");
    ASSERT_EQ(suite, 3, vr.reg, "Local → reg should be 3");
    ASSERT_FALSE(suite, vr.ownsRegister, "Local should not own register");
}

void testSymbolToValueUpvalue(TestSuite& suite) {
    SymbolRef sym;
    sym.kind = SymbolRef::Kind::Upvalue;
    sym.index = 0;
    sym.name = "x";
    StringPool& pool = StringPool::getInstance();
    CodeGenerator codegen(&pool);
    ValueResult vr = codegen.symbolToValue(sym);
    ASSERT_TRUE(suite, vr.kind == ValueResult::Kind::PendingLoad, "Upvalue → PendingLoad kind");
    ASSERT_TRUE(suite, vr.access == ValueResult::AccessKind::Upvalue, "Upvalue → Upvalue access");
    ASSERT_EQ(suite, 0, vr.aux, "Upvalue → aux should be 0");
}

void testSymbolToValueGlobal(TestSuite& suite) {
    SymbolRef sym;
    sym.kind = SymbolRef::Kind::Global;
    sym.index = 5;
    sym.name = "print";
    StringPool& pool = StringPool::getInstance();
    CodeGenerator codegen(&pool);
    ValueResult vr = codegen.symbolToValue(sym);
    ASSERT_TRUE(suite, vr.kind == ValueResult::Kind::PendingLoad, "Global → PendingLoad kind");
    ASSERT_TRUE(suite, vr.access == ValueResult::AccessKind::Global, "Global → Global access");
    ASSERT_EQ(suite, 5, vr.constIndex, "Global → constIndex should be 5");
}

void testSymbolToLValueLocal(TestSuite& suite) {
    SymbolRef sym;
    sym.kind = SymbolRef::Kind::Local;
    sym.index = 2;
    StringPool& pool = StringPool::getInstance();
    CodeGenerator codegen(&pool);
    LValueRef lv = codegen.symbolToLValue(sym);
    ASSERT_TRUE(suite, lv.kind == LValueRef::Kind::Local, "LValue Local kind");
    ASSERT_EQ(suite, 2, lv.slot, "LValue Local slot is 2");
}

void testSymbolToLValueUpvalue(TestSuite& suite) {
    SymbolRef sym;
    sym.kind = SymbolRef::Kind::Upvalue;
    sym.index = 1;
    StringPool& pool = StringPool::getInstance();
    CodeGenerator codegen(&pool);
    LValueRef lv = codegen.symbolToLValue(sym);
    ASSERT_TRUE(suite, lv.kind == LValueRef::Kind::Upvalue, "LValue Upvalue kind");
    ASSERT_EQ(suite, 1, lv.slot, "LValue Upvalue slot is 1");
}

void testSymbolToLValueGlobal(TestSuite& suite) {
    SymbolRef sym;
    sym.kind = SymbolRef::Kind::Global;
    sym.index = 3;
    StringPool& pool = StringPool::getInstance();
    CodeGenerator codegen(&pool);
    LValueRef lv = codegen.symbolToLValue(sym);
    ASSERT_TRUE(suite, lv.kind == LValueRef::Kind::Global, "LValue Global kind");
    ASSERT_EQ(suite, 3, lv.slot, "LValue Global slot is 3");
}

// =============================================================================
// 字节码级验证：名字解析 → 正确指令选择
// =============================================================================

void testLocalVarReadNoGetglobal(TestSuite& suite) {
    int c = countOpcode("local x = 42\nreturn x", OpCode::GETGLOBAL);
    ASSERT_EQ(suite, 0, c, "local read should not emit GETGLOBAL");
}

void testGlobalVarReadUsesGetglobal(TestSuite& suite) {
    int c = countOpcode("return my_global", OpCode::GETGLOBAL);
    ASSERT_EQ(suite, 1, c, "global read should emit GETGLOBAL");
}

void testLocalAssignNoSetglobal(TestSuite& suite) {
    int c = countOpcode("local x\nx = 10", OpCode::SETGLOBAL);
    ASSERT_EQ(suite, 0, c, "local assign should not emit SETGLOBAL");
}

void testGlobalAssignUsesSetglobal(TestSuite& suite) {
    int c = countOpcode("g = 123", OpCode::SETGLOBAL);
    ASSERT_EQ(suite, 1, c, "global assign should emit SETGLOBAL");
}

// =============================================================================
// 运行时验证：名字解析 → 正确语义
// =============================================================================

void testLocalReadWriteRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, "local x = 42\n_result = x");
    ASSERT_TRUE(suite, ok, "local read/write should run");
    ASSERT_EQ(suite, 42.0, getGlobalNumber(L, "_result"), "result should be 42.0");
    delete L;
}

void testGlobalReadWriteRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, "g = 99\n_result = g");
    ASSERT_TRUE(suite, ok, "global read/write should run");
    ASSERT_EQ(suite, 99.0, getGlobalNumber(L, "_result"), "result should be 99.0");
    delete L;
}

void testLocalShadowsGlobal(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, "myvar = \"global\"\nlocal myvar = \"local\"\n_result = myvar");
    ASSERT_TRUE(suite, ok, "local shadow should run");
    ASSERT_TRUE(suite, getGlobalString(L, "_result") == "local", "result should be 'local'");
    delete L;
}

void testUpvalueCaptureRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, "local outer = 10\nlocal f = function() _result = outer end\nf()");
    ASSERT_TRUE(suite, ok, "upvalue capture should run");
    ASSERT_EQ(suite, 10.0, getGlobalNumber(L, "_result"), "result should be 10.0");
    delete L;
}

void testUpvalueWritebackRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L,
        "local outer = 10\n"
        "local f = function() outer = 20 end\n"
        "f()\n_result = outer");
    ASSERT_TRUE(suite, ok, "upvalue writeback should run");
    ASSERT_EQ(suite, 20.0, getGlobalNumber(L, "_result"), "result should be 20.0");
    delete L;
}

void testNestedUpvalueChain(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L,
        "local a = 1\n"
        "local f1 = function()\n"
        "  local f2 = function() _result = a end\n"
        "  return f2\n"
        "end\n"
        "local f = f1()\nf()");
    ASSERT_TRUE(suite, ok, "nested upvalue chain should run");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "_result"), "result should be 1.0");
    delete L;
}

// =============================================================================
// FunctionStmt 表路径加载 — 通过 resolve() 收敛
// =============================================================================

void testFunctionTablePathResolve(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L,
        "t = {}\nt.a = {}\nt.a.b = {}\n"
        "function t.a.b:foo() _result = 1234 end\n"
        "t.a.b:foo()");
    ASSERT_TRUE(suite, ok, "table path function should run");
    ASSERT_EQ(suite, 1234.0, getGlobalNumber(L, "_result"), "result should be 1234.0");
    delete L;
}

void testGlobalFunctionDefinition(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, "function myGlobalFunc() _result = 5678 end\nmyGlobalFunc()");
    ASSERT_TRUE(suite, ok, "global function should run");
    ASSERT_EQ(suite, 5678.0, getGlobalNumber(L, "_result"), "result should be 5678.0");
    delete L;
}

void testLocalFunctionDefinition(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, "local function myLocalFunc() _result = 999 end\nmyLocalFunc()");
    ASSERT_TRUE(suite, ok, "local function should run");
    ASSERT_EQ(suite, 999.0, getGlobalNumber(L, "_result"), "result should be 999.0");
    delete L;
}

// =============================================================================
// 注册
// =============================================================================

void registerSymbolBindingTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Symbol Binding (PR-8)", "SymbolRef default is None", testSymbolRefDefaultInvalid);
    registry.registerTest("Symbol Binding (PR-8)", "symbolToValue Local → Register", testSymbolToValueLocal);
    registry.registerTest("Symbol Binding (PR-8)", "symbolToValue Upvalue → PendingLoad", testSymbolToValueUpvalue);
    registry.registerTest("Symbol Binding (PR-8)", "symbolToValue Global → PendingLoad", testSymbolToValueGlobal);
    registry.registerTest("Symbol Binding (PR-8)", "symbolToLValue Local", testSymbolToLValueLocal);
    registry.registerTest("Symbol Binding (PR-8)", "symbolToLValue Upvalue", testSymbolToLValueUpvalue);
    registry.registerTest("Symbol Binding (PR-8)", "symbolToLValue Global", testSymbolToLValueGlobal);

    registry.registerTest("Symbol Binding (PR-8)", "local var read → no GETGLOBAL", testLocalVarReadNoGetglobal);
    registry.registerTest("Symbol Binding (PR-8)", "global var read → GETGLOBAL", testGlobalVarReadUsesGetglobal);
    registry.registerTest("Symbol Binding (PR-8)", "local assign → no SETGLOBAL", testLocalAssignNoSetglobal);
    registry.registerTest("Symbol Binding (PR-8)", "global assign → SETGLOBAL", testGlobalAssignUsesSetglobal);

    registry.registerTest("Symbol Binding (PR-8)", "local read/write runtime", testLocalReadWriteRuntime);
    registry.registerTest("Symbol Binding (PR-8)", "global read/write runtime", testGlobalReadWriteRuntime);
    registry.registerTest("Symbol Binding (PR-8)", "local shadows global", testLocalShadowsGlobal);
    registry.registerTest("Symbol Binding (PR-8)", "upvalue capture runtime", testUpvalueCaptureRuntime);
    registry.registerTest("Symbol Binding (PR-8)", "upvalue writeback runtime", testUpvalueWritebackRuntime);
    registry.registerTest("Symbol Binding (PR-8)", "nested upvalue chain", testNestedUpvalueChain);

    registry.registerTest("Symbol Binding (PR-8)", "function table path resolve", testFunctionTablePathResolve);
    registry.registerTest("Symbol Binding (PR-8)", "global function definition", testGlobalFunctionDefinition);
    registry.registerTest("Symbol Binding (PR-8)", "local function definition", testLocalFunctionDefinition);
}
