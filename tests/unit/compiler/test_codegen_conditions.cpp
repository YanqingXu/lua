/**
 * @file test_codegen_conditions.cpp
 * @brief Guardrail tests for condition and short-circuit semantics before removing expdesc.
 */

#include "../framework/test_framework.hpp"
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

constexpr const char* kSuiteName = "Codegen Conditions";

Proto* generateProto(const char* code) {
    StringPool& pool = StringPool::getInstance();
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    return codegen.generate(chunk, "test_codegen_conditions");
}

bool runLua(LuaState* L, const char* code) {
    try {
        Parser parser(code);
        Chunk chunk = parser.parse();
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, "test_codegen_conditions");
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

bool hasOpcode(const Proto* proto, OpCode op) {
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == op) {
            return true;
        }
    }
    return false;
}

bool hasPendingJump(const Proto* proto) {
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::JMP && GETARG_sBx(inst) == -1) {
            return true;
        }
    }
    return false;
}

bool hasResolvedTestJumpPattern(const Proto* proto) {
    for (usize i = 0; i + 1 < proto->getInstructionCount(); i++) {
        Instruction testInst = proto->getInstruction(i);
        Instruction jmpInst = proto->getInstruction(i + 1);

        if (GET_OPCODE(testInst) != OpCode::TEST) {
            continue;
        }
        if (GET_OPCODE(jmpInst) != OpCode::JMP) {
            continue;
        }
        if (GETARG_sBx(jmpInst) == -1) {
            continue;
        }
        return true;
    }
    return false;
}

} // namespace

void testShortCircuitRuntime(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local rhsCalls = 0
        local function mark(v)
            rhsCalls = rhsCalls + 1
            return v
        end

        local andSkip = false and mark("and-skip")
        assert(andSkip == false, "false and rhs should stay false")
        assert(rhsCalls == 0, "false and rhs should not execute")

        local andTake = true and mark("and-take")
        assert(andTake == "and-take", "true and rhs should return rhs")
        assert(rhsCalls == 1, "true and rhs should execute exactly once")

        local nilSkip = nil and mark("nil-skip")
        assert(nilSkip == nil, "nil and rhs should stay nil")
        assert(rhsCalls == 1, "nil and rhs should not execute")

        local zeroTake = 0 and mark("zero-is-truthy")
        assert(zeroTake == "zero-is-truthy", "0 should be truthy in Lua")
        assert(rhsCalls == 2, "truthy zero should evaluate rhs")

        local orSkip = true or mark("or-skip")
        assert(orSkip == true, "true or rhs should keep lhs")
        assert(rhsCalls == 2, "true or rhs should not execute")

        local orTake = false or mark("or-take")
        assert(orTake == "or-take", "false or rhs should return rhs")
        assert(rhsCalls == 3, "false or rhs should execute")

        assert((not nil) == true, "not nil should be true")
        assert((not false) == true, "not false should be true")
        assert((not 0) == false, "not truthy value should be false")
    )lua");

    ASSERT_TRUE(suite, ok, "Short-circuit runtime semantics");

    delete L;
}

void testConditionContextsRuntime(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local calls = 0
        local function mark(v)
            calls = calls + 1
            return v
        end

        local branch = 0
        if false and mark(true) then
            branch = -100
        else
            branch = branch + 1
        end

        if true or mark(false) then
            branch = branch + 10
        end

        local whileCount = 0
        while whileCount < 2 and mark(true) do
            whileCount = whileCount + 1
        end

        local repeatCount = 0
        repeat
            repeatCount = repeatCount + 1
        until repeatCount == 2 or mark(false)

        assert(branch == 11, "if conditions should take expected branches")
        assert(whileCount == 2, "while condition should loop twice")
        assert(repeatCount == 2, "repeat-until condition should stop on second iteration")
        assert(calls == 3, "only the live rhs branches should execute")
    )lua");

    ASSERT_TRUE(suite, ok, "Condition contexts runtime semantics");

    delete L;
}

void testConditionBytecodeHasResolvedJumps(TestSuite& suite) {
    const char* code =
        "local a = ...\n"
        "local b = ...\n"
        "if a and b then return 1 end\n"
        "while a or b do break end\n"
        "repeat a = false until not a\n"
        "return 0\n";

    Proto* proto = generateProto(code);

    ASSERT_TRUE(suite, proto != nullptr, "Condition proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Condition proto has instructions");
    ASSERT_TRUE(suite, hasOpcode(proto, OpCode::TEST), "Condition bytecode uses TEST");
    ASSERT_TRUE(suite, hasResolvedTestJumpPattern(proto), "Condition TEST/JMP pattern is resolved");
    ASSERT_FALSE(suite, hasPendingJump(proto), "Condition bytecode has no pending JMP");

    delete proto;
}

void testNestedNotConditionUsesCondPipeline(TestSuite& suite) {
    const char* code =
        "local a = ...\n"
        "local b = ...\n"
        "if not (a and b) then return 1 end\n"
        "return 0\n";

    Proto* proto = generateProto(code);

    ASSERT_TRUE(suite, proto != nullptr, "Nested not condition proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Nested not condition has instructions");
    ASSERT_TRUE(suite, hasOpcode(proto, OpCode::TEST), "Nested not condition uses TEST");
    ASSERT_FALSE(suite, hasOpcode(proto, OpCode::NOT), "Nested not condition avoids OP_NOT");
    ASSERT_FALSE(suite, hasPendingJump(proto), "Nested not condition has no pending JMP");

    delete proto;
}

void registerCodegenConditionTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Short Circuit Runtime", testShortCircuitRuntime);
    registry.registerTest(kSuiteName, "Condition Contexts Runtime", testConditionContextsRuntime);
    registry.registerTest(kSuiteName, "Condition Bytecode Has Resolved Jumps", testConditionBytecodeHasResolvedJumps);
    registry.registerTest(kSuiteName, "Nested Not Condition Uses Cond Pipeline", testNestedNotConditionUsesCondPipeline);
}
