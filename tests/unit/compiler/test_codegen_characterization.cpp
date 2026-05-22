/**
 * @file test_codegen_characterization.cpp
 * @brief Characterization tests for CodeGenerator statement and jump boundaries.
 */

#include "../framework/test_framework.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/opcode.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/string_pool.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Codegen Characterization";

Proto* generateProto(const char* code) {
    StringPool& pool = StringPool::getInstance();
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    return codegen.generate(chunk, "test_codegen_characterization");
}

LuaState* executeChunk(const char* code) {
    Proto* proto = generateProto(code);
    if (proto == nullptr) {
        return nullptr;
    }

    LuaState* L = LuaState::newState();
    Function* func = new Function(proto);
    L->getGlobalState().getGC().registerObject(func);
    func->setEnv(L->getGlobalTable());
    VM::execute(L, func);
    return L;
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

usize countOpcode(const Proto* proto, OpCode op) {
    usize count = 0;
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == op) {
            count++;
        }
    }
    return count;
}

bool hasForwardJump(const Proto* proto) {
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::JMP && GETARG_sBx(inst) > 0) {
            return true;
        }
    }
    return false;
}

bool hasBackwardJump(const Proto* proto) {
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::JMP && GETARG_sBx(inst) < 0) {
            return true;
        }
    }
    return false;
}

bool hasTForLoopBackEdge(const Proto* proto) {
    for (usize tforPc = 0; tforPc < proto->getInstructionCount(); tforPc++) {
        if (GET_OPCODE(proto->getInstruction(tforPc)) != OpCode::TFORLOOP) {
            continue;
        }

        for (usize pc = tforPc + 1; pc < proto->getInstructionCount(); pc++) {
            Instruction inst = proto->getInstruction(pc);
            if (GET_OPCODE(inst) != OpCode::JMP || GETARG_sBx(inst) >= 0) {
                continue;
            }

            const i32 target = static_cast<i32>(pc) + 1 + GETARG_sBx(inst);
            if (target <= static_cast<i32>(tforPc)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

void testStatementLoweringRuntimeKeepsLoopAndScopeSemantics(TestSuite& suite) {
    LuaState* L = nullptr;
    try {
        L = executeChunk(R"lua(
            local total = 0
            for i = 1, 5 do
                if i == 4 then break end
                total = total + i
            end

            local repeatValue = 0
            repeat
                local done = repeatValue >= 2
                repeatValue = repeatValue + 1
            until done

            local whileValue = 0
            while true do
                whileValue = whileValue + 1
                if whileValue == 3 then break end
            end

            do
                local hidden = 9
                total = total + hidden
            end

            return total, repeatValue, whileValue
        )lua");
    } catch (...) {
        L = nullptr;
    }

    ASSERT_TRUE(suite, L != nullptr, "Statement lowering chunk executes");
    if (L != nullptr) {
        ASSERT_TRUE(suite, L->getTop() >= 3, "Statement lowering returns three values");
        if (L->getTop() >= 3) {
            ASSERT_TRUE(suite, L->at(-3).isNumber(), "Numeric for and do block result is numeric");
            ASSERT_TRUE(suite, L->at(-2).isNumber(), "Repeat-until result is numeric");
            ASSERT_TRUE(suite, L->at(-1).isNumber(), "While break result is numeric");
            ASSERT_EQ(suite, 15.0, L->at(-3).asNumber(), "Numeric for break and do block total");
            ASSERT_EQ(suite, 3.0, L->at(-2).asNumber(), "Repeat body local is visible to until condition");
            ASSERT_EQ(suite, 3.0, L->at(-1).asNumber(), "While break exits at expected iteration");
        }
    }

    delete L;
}

void testStructuredStatementsLeaveNoPendingJumps(TestSuite& suite) {
    Proto* proto = generateProto(R"lua(
        local a = ...
        local total = 0
        if a then
            total = total + 1
        elseif total == 0 then
            total = total + 2
        else
            total = total + 3
        end

        while total < 8 do
            total = total + 1
            if total == 5 then break end
        end

        repeat
            total = total + 1
        until total >= 6

        for i = 1, 3 do
            total = total + i
        end

        return total
    )lua");

    ASSERT_TRUE(suite, proto != nullptr, "Structured statement proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Structured statement proto has instructions");
    ASSERT_FALSE(suite, hasPendingJump(proto), "Structured statements leave no pending JMP");
    ASSERT_TRUE(suite, hasForwardJump(proto), "Structured statements include a resolved forward JMP");
    ASSERT_TRUE(suite, hasBackwardJump(proto), "Structured statements include a resolved backward JMP");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::FORPREP) == 1, "Numeric for emits FORPREP");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::FORLOOP) == 1, "Numeric for emits FORLOOP");

    delete proto;
}

void testGenericForBytecodeShapeIsStable(TestSuite& suite) {
    Proto* proto = generateProto(R"lua(
        local t = {}
        local seen = 0
        for k, v in next, t, nil do
            seen = seen + 1
            if k then break end
        end
        return seen
    )lua");

    ASSERT_TRUE(suite, proto != nullptr, "Generic for proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Generic for proto has instructions");
    ASSERT_FALSE(suite, hasPendingJump(proto), "Generic for leaves no pending JMP");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::TFORLOOP) == 1, "Generic for emits one TFORLOOP");
    ASSERT_TRUE(suite, hasTForLoopBackEdge(proto), "Generic for keeps TFORLOOP back edge");

    delete proto;
}

void registerCodegenCharacterizationTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Statement Lowering Runtime Keeps Loop And Scope Semantics",
                          testStatementLoweringRuntimeKeepsLoopAndScopeSemantics);
    registry.registerTest(kSuiteName, "Structured Statements Leave No Pending Jumps",
                          testStructuredStatementsLeaveNoPendingJumps);
    registry.registerTest(kSuiteName, "Generic For Bytecode Shape Is Stable",
                          testGenericForBytecodeShapeIsStable);
}
