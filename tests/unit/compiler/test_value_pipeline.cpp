/**
 * @file test_value_pipeline.cpp
 * @brief PR-4 ValueResult Pipeline unit tests
 *
 * Tests the emitValue / dischargeValue / valueToRK / valueToAnyReg / forceSingleValue pipeline.
 * Covers literals, name reads, paren expressions, RK encoding, and runtime correctness.
 */

#include "../framework/test_framework.hpp"
#include "compiler/parser/lexer.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/codegen/codegen_types.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "lib/lib_manager.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "ValueResult Pipeline";

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
        Proto* proto = codegen.generate(chunk, "test_value_pipeline");
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

Proto* generateProto(const char* code) {
    StringPool& pool = StringPool::getInstance();
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);
    CodeGenerator codegen(&pool);
    return codegen.generate(chunk);
}

int countOpcode(const char* code, OpCode op) {
    Proto* proto = generateProto(code);
    int count = 0;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == op)
            count++;
    }
    delete proto;
    return count;
}

bool hasOpcode(const char* code, OpCode op) {
    return countOpcode(code, op) > 0;
}

}  // namespace

// =====================================================================
// Bytecode-level tests for literal ValueResult
// =====================================================================

void testValueNilBytecode(TestSuite& suite) {
    // local x = nil -> LOADNIL
    ASSERT_TRUE(suite, hasOpcode("local x = nil", OpCode::LOADNIL),
                "nil literal generates LOADNIL");
}

void testValueBoolBytecode(TestSuite& suite) {
    // local x = true -> LOADBOOL
    ASSERT_TRUE(suite, hasOpcode("local x = true", OpCode::LOADBOOL),
                "true literal generates LOADBOOL");
    ASSERT_TRUE(suite, hasOpcode("local x = false", OpCode::LOADBOOL),
                "false literal generates LOADBOOL");
}

void testValueNumberBytecode(TestSuite& suite) {
    // local x = 42 -> LOADK
    ASSERT_TRUE(suite, hasOpcode("local x = 42", OpCode::LOADK),
                "number literal generates LOADK");
}

void testValueStringBytecode(TestSuite& suite) {
    // local x = "hello" -> LOADK
    ASSERT_TRUE(suite, hasOpcode("local x = \"hello\"", OpCode::LOADK),
                "string literal generates LOADK");
}

void testValueLocalReadBytecode(TestSuite& suite) {
    // local a = 1; local b = a -> might need MOVE or not depending on reg
    Proto* proto = generateProto("local a = 1\nlocal b = a");
    bool foundMove = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::MOVE) {
            foundMove = true;
            break;
        }
    }
    // b = a where a is local may produce MOVE (if different registers)
    ASSERT_TRUE(suite, foundMove, "local read may generate MOVE");
    delete proto;
}

void testValueGlobalReadBytecode(TestSuite& suite) {
    // local x = foo -> GETGLOBAL
    ASSERT_TRUE(suite, hasOpcode("local x = foo", OpCode::GETGLOBAL),
                "global read generates GETGLOBAL");
}

void testValueUpvalueReadBytecode(TestSuite& suite) {
    // upvalue read generates GETUPVAL in the inner proto
    const char* code = R"(
        local a = 1
        local f = function() return a end
    )";
    Proto* outerProto = generateProto(code);
    // Inner function should have GETUPVAL
    ASSERT_TRUE(suite, outerProto->getSubProtoCount() > 0,
                "inner function proto exists");
    Proto* innerProto = outerProto->getSubProto(0);
    bool found = false;
    for (size_t i = 0; i < innerProto->getInstructionCount(); i++) {
        if (GET_OPCODE(innerProto->getInstruction(i)) == OpCode::GETUPVAL)
            found = true;
    }
    ASSERT_TRUE(suite, found, "upvalue read generates GETUPVAL in inner proto");
    delete outerProto;
}

// =====================================================================
// Bytecode-level tests for RK selection
// =====================================================================

void testValueRKConstantEncoding(TestSuite& suite) {
    // a + 1 -> ADD uses RK for the constant 1
    const char* code = "local a = 10\nlocal b = a + 1";
    Proto* proto = generateProto(code);
    bool foundAdd = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::ADD) {
            foundAdd = true;
            // Either B or C should be an RK constant (bit 8 set)
            i32 b = GETARG_B(inst);
            i32 c = GETARG_C(inst);
            ASSERT_TRUE(suite, ISK(b) || ISK(c),
                        "ADD uses RK constant for literal operand");
            break;
        }
    }
    ASSERT_TRUE(suite, foundAdd, "a + 1 generates ADD instruction");
    delete proto;
}

// =====================================================================
// ParenExpr tests (forceSingle semantics)
// =====================================================================

void testValueParenSingleBytecode(TestSuite& suite) {
    // (a) should be same as a
    const char* code = "local a = 10\nlocal b = (a)";
    ASSERT_TRUE(suite, hasOpcode(code, OpCode::MOVE),
                "parenthesized local generates MOVE");
}

void testValueParenCallSingle(TestSuite& suite) {
    // (f()) should collapse multret to single value
    const char* code = R"(
        local function f() return 1, 2, 3 end
        local x = (f())
    )";
    Proto* proto = generateProto(code);
    // The CALL in the outer proto should have C=2 (1 return value)
    bool foundCall = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::CALL) {
            foundCall = true;
            i32 c = GETARG_C(inst);
            ASSERT_EQ(suite, 2, c, "(f()) CALL has C=2 (1 return value)");
            break;
        }
    }
    ASSERT_TRUE(suite, foundCall, "(f()) generates CALL");
    delete proto;
}

// =====================================================================
// FunctionExpr tests
// =====================================================================

void testValueFunctionExprBytecode(TestSuite& suite) {
    const char* code = "local f = function(x) return x end";
    ASSERT_TRUE(suite, hasOpcode(code, OpCode::CLOSURE),
                "function expression generates CLOSURE");
}

// =====================================================================
// Runtime correctness tests via emitValue
// =====================================================================

void testValueLiteralsRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local n = nil
        local t = true
        local f = false
        local num = 42
        local str = "hello"
        assert(n == nil)
        assert(t == true)
        assert(f == false)
        assert(num == 42)
        assert(str == "hello")
    )");
    ASSERT_TRUE(suite, ok, "all literal types produce correct runtime values");
    delete L;
}

void testValueNameReadRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local a = 10
        local b = 20
        local c = a
        local d = b
        assert(c == 10)
        assert(d == 20)
    )");
    ASSERT_TRUE(suite, ok, "local name reads produce correct values");
    delete L;
}

void testValueGlobalReadRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        g = 99
        local x = g
        assert(x == 99)
    )");
    ASSERT_TRUE(suite, ok, "global name read produces correct value");
    delete L;
}

void testValueParenRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local a = 5
        local b = (a)
        local c = ((a))
        assert(b == 5)
        assert(c == 5)
    )");
    ASSERT_TRUE(suite, ok, "parenthesized expressions preserve value");
    delete L;
}

void testValueParenCallRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local function f() return 1, 2, 3 end
        local x = (f())
        assert(x == 1, "paren call returns only first value")
        local a, b = (f())
        assert(a == 1)
        assert(b == nil, "(f()) should produce single value, b should be nil")
    )");
    ASSERT_TRUE(suite, ok, "(f()) collapses multret to single value at runtime");
    delete L;
}

void testValueDoubleParenRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local a = 42
        local b = ((a))
        assert(b == 42, "double paren preserves value")
    )");
    ASSERT_TRUE(suite, ok, "double paren ((a)) works correctly");
    delete L;
}

void testValueFunctionExprRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local f = function(x) return x + 1 end
        assert(f(10) == 11)
    )");
    ASSERT_TRUE(suite, ok, "function expression binds and executes correctly");
    delete L;
}

void testValueArithRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local a = 10
        local b = a + 5
        local c = b * 2
        local d = c - 1
        local e = d / 3
        assert(b == 15)
        assert(c == 30)
        assert(d == 29)
    )");
    ASSERT_TRUE(suite, ok, "arithmetic chain produces correct values");
    delete L;
}

void testValueStringConcatRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local s = "hello" .. " " .. "world"
        assert(s == "hello world")
    )");
    ASSERT_TRUE(suite, ok, "string concat produces correct result");
    delete L;
}

void testValueTableIndexRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local t = {a = 10, b = 20}
        local x = t.a
        local y = t["b"]
        assert(x == 10)
        assert(y == 20)
    )");
    ASSERT_TRUE(suite, ok, "table index read produces correct values");
    delete L;
}

void testValueCallResultRuntime(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"(
        local function pair() return 1, 2 end
        local a = pair()
        assert(a == 1, "call result single captures first value")
        local b, c = pair()
        assert(b == 1)
        assert(c == 2)
    )");
    ASSERT_TRUE(suite, ok, "call result correctly captured in locals");
    delete L;
}

// =====================================================================
// Registration
// =====================================================================

void registerValuePipelineTests() {
    auto& registry = TestRegistry::getInstance();

    // Bytecode tests
    registry.registerTest(kSuiteName, "Nil Bytecode", testValueNilBytecode);
    registry.registerTest(kSuiteName, "Bool Bytecode", testValueBoolBytecode);
    registry.registerTest(kSuiteName, "Number Bytecode", testValueNumberBytecode);
    registry.registerTest(kSuiteName, "String Bytecode", testValueStringBytecode);
    registry.registerTest(kSuiteName, "Local Read Bytecode", testValueLocalReadBytecode);
    registry.registerTest(kSuiteName, "Global Read Bytecode", testValueGlobalReadBytecode);
    registry.registerTest(kSuiteName, "Upvalue Read Bytecode", testValueUpvalueReadBytecode);
    registry.registerTest(kSuiteName, "RK Constant Encoding", testValueRKConstantEncoding);
    registry.registerTest(kSuiteName, "Paren Single Bytecode", testValueParenSingleBytecode);
    registry.registerTest(kSuiteName, "Paren Call Single", testValueParenCallSingle);
    registry.registerTest(kSuiteName, "FunctionExpr Bytecode", testValueFunctionExprBytecode);

    // Runtime tests
    registry.registerTest(kSuiteName, "Literals Runtime", testValueLiteralsRuntime);
    registry.registerTest(kSuiteName, "Name Read Runtime", testValueNameReadRuntime);
    registry.registerTest(kSuiteName, "Global Read Runtime", testValueGlobalReadRuntime);
    registry.registerTest(kSuiteName, "Paren Runtime", testValueParenRuntime);
    registry.registerTest(kSuiteName, "Paren Call Runtime", testValueParenCallRuntime);
    registry.registerTest(kSuiteName, "Double Paren Runtime", testValueDoubleParenRuntime);
    registry.registerTest(kSuiteName, "FunctionExpr Runtime", testValueFunctionExprRuntime);
    registry.registerTest(kSuiteName, "Arithmetic Runtime", testValueArithRuntime);
    registry.registerTest(kSuiteName, "String Concat Runtime", testValueStringConcatRuntime);
    registry.registerTest(kSuiteName, "Table Index Runtime", testValueTableIndexRuntime);
    registry.registerTest(kSuiteName, "Call Result Runtime", testValueCallResultRuntime);
}
