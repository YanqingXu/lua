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

bool hasABC(const Proto* proto, OpCode op, i32 a, i32 b, i32 c) {
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == op && GETARG_A(inst) == a && GETARG_B(inst) == b &&
            GETARG_C(inst) == c) {
            return true;
        }
    }
    return false;
}

bool hasReturn(const Proto* proto, i32 a, i32 b) {
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::RETURN && GETARG_A(inst) == a && GETARG_B(inst) == b) {
            return true;
        }
    }
    return false;
}

bool hasSetTableFromValueRegister(const Proto* proto, i32 tableReg, i32 valueReg) {
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::SETTABLE && GETARG_A(inst) == tableReg &&
            GETARG_C(inst) == valueReg) {
            return true;
        }
    }
    return false;
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

}

void testLua51LoadNilMergeAndDirectReturnParityGapIsCharacterized(TestSuite& suite) {
    Proto* proto = generateProto("local a,b,c = nil,nil,nil; return a,b,c");

    ASSERT_TRUE(suite, proto != nullptr, "Explicit nil local proto generated");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::LOADNIL) == 1,
                "Explicit nil locals merge into one LOADNIL instruction");
    ASSERT_TRUE(suite, hasABC(proto, OpCode::LOADNIL, 0, 2, 0),
                "Explicit nil locals use a Lua 5.1-style LOADNIL register range");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::MOVE) == 0,
                "Direct local return reuses contiguous local registers without MOVE");
    ASSERT_FALSE(suite, hasReturn(proto, 3, 4), "Multi-return no longer starts at temporary register 3");
    ASSERT_TRUE(suite, hasReturn(proto, 0, 4),
                "Lua 5.1-style direct local multi-return starts at register 0");


    Proto* elided = generateProto(R"lua(
        local a,b,c
        local d; local e
    )lua");

    ASSERT_TRUE(suite, elided != nullptr, "Dead nil initialization proto generated");
    ASSERT_TRUE(suite, countOpcode(elided, OpCode::LOADNIL) == 0,
                "Lua 5.1-style dead nil local initialization emits no LOADNIL");
    ASSERT_TRUE(suite, countOpcode(elided, OpCode::MOVE) == 0,
                "Lua 5.1-style dead nil local initialization emits no MOVE");
    ASSERT_TRUE(suite, countOpcode(elided, OpCode::RETURN) == 1,
                "Dead nil local chunk lowers to the final RETURN only");

    delete elided;

    Proto* preserved = generateProto(R"lua(
        local a,b,c
        local d; local e
        a = nil; d = nil
    )lua");

    ASSERT_TRUE(suite, preserved != nullptr, "Nil assignment proto generated");
    ASSERT_TRUE(suite, countOpcode(preserved, OpCode::LOADNIL) >= 1,
                "Ordinary nil assignments are preserved instead of path-insensitive dead-store elided");
    ASSERT_TRUE(suite, countOpcode(preserved, OpCode::RETURN) == 1,
                "Nil assignment chunk still has one final RETURN");

    delete preserved;

    LuaState* captured = nullptr;
    try {
        captured = executeChunk(R"lua(
            local a = 1
            local f = function() return a end
            a = nil
            return f()
        )lua");
    } catch (...) {
        captured = nullptr;
    }

    ASSERT_TRUE(suite, captured != nullptr, "Captured local nil assignment chunk executes");
    if (captured != nullptr) {
        ASSERT_TRUE(suite, captured->getTop() >= 1, "Captured local nil assignment returns a value");
        if (captured->getTop() >= 1) {
            ASSERT_TRUE(suite, captured->at(-1).isNil(),
                        "Nil assignment to captured local is preserved for escaped closures");
        }
    }
    delete captured;
}

void testLua51NilAssignmentInLoopRemainsObservable(TestSuite& suite) {
    LuaState* L = nullptr;
    try {
        L = executeChunk(R"lua(
            local function f(x)
                local first = 1
                local guard = 0
                while 1 do
                    guard = guard + 1
                    if guard > 4 then
                        return "loop"
                    end
                    if x == 3 and not first then
                        return "done"
                    end
                    first = nil
                end
            end

            return f(3)
        )lua");
    } catch (...) {
        L = nullptr;
    }

    ASSERT_TRUE(suite, L != nullptr, "Loop nil assignment chunk executes");
    if (L != nullptr) {
        ASSERT_TRUE(suite, L->getTop() >= 1, "Loop nil assignment returns one value");
        if (L->getTop() >= 1) {
            const Value result = L->at(-1);
            ASSERT_TRUE(suite, result.isString(), "Loop nil assignment result is a string");
            if (result.isString()) {
                ASSERT_EQ(suite, std::string("done"), std::string(result.asString()->c_str()),
                          "Nil assignment inside a loop is visible on the next iteration");
            }
        }
    }

    delete L;
}

void testLua51ConcatMergeParityGapIsCharacterized(TestSuite& suite) {
    Proto* proto = generateProto(R"lua(
        local a,b,c = "a","b","c"
        return a .. b .. c
    )lua");

    ASSERT_TRUE(suite, proto != nullptr, "Concat proto generated");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::CONCAT) == 1,
                "Lua 5.1-style chained concat lowers as one CONCAT instruction");
    ASSERT_TRUE(suite, hasABC(proto, OpCode::CONCAT, 3, 3, 5),
                "Merged concat uses a scratch operand range so source locals stay intact");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::MOVE) == 3,
                "Merged concat copies active locals before VM CONCAT mutates the operand range");

}

void testLua51NotNotBooleanNormalizationShapeIsCharacterized(TestSuite& suite) {
    Proto* nilProto = generateProto("return not not nil");

    ASSERT_TRUE(suite, nilProto != nullptr, "constant not-not nil proto generated");
    ASSERT_TRUE(suite, countOpcode(nilProto, OpCode::LOADBOOL) == 1,
                "Lua 5.1-style not-not nil folds to one LOADBOOL");
    ASSERT_TRUE(suite, countOpcode(nilProto, OpCode::JMP) == 0,
                "Lua 5.1-style not-not nil emits no JMP");

    delete nilProto;

    Proto* falseProto = generateProto("return not not false");

    ASSERT_TRUE(suite, falseProto != nullptr, "constant not-not false proto generated");
    ASSERT_TRUE(suite, countOpcode(falseProto, OpCode::LOADBOOL) == 1,
                "Lua 5.1-style not-not false folds to one LOADBOOL");
    ASSERT_TRUE(suite, countOpcode(falseProto, OpCode::JMP) == 0,
                "Lua 5.1-style not-not false emits no JMP");

    delete falseProto;

    Proto* trueProto = generateProto("return not not true");

    ASSERT_TRUE(suite, trueProto != nullptr, "constant not-not true proto generated");
    ASSERT_TRUE(suite, countOpcode(trueProto, OpCode::LOADBOOL) == 1,
                "Lua 5.1-style not-not true folds to one LOADBOOL");
    ASSERT_TRUE(suite, countOpcode(trueProto, OpCode::JMP) == 0,
                "Lua 5.1-style not-not true emits no JMP");

    delete trueProto;

    Proto* numberProto = generateProto("return not not 1");

    ASSERT_TRUE(suite, numberProto != nullptr, "constant not-not number proto generated");
    ASSERT_TRUE(suite, countOpcode(numberProto, OpCode::LOADBOOL) == 1,
                "Lua 5.1-style not-not number folds to one LOADBOOL");
    ASSERT_TRUE(suite, countOpcode(numberProto, OpCode::JMP) == 0,
                "Lua 5.1-style not-not number emits no JMP");

    delete numberProto;

    Proto* proto = generateProto("local a = ...; return not not a");

    ASSERT_TRUE(suite, proto != nullptr, "not-not proto generated");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::TEST) == 1,
                "Current not-not lowering uses one TEST instruction");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::JMP) == 1,
                "Current not-not lowering uses one JMP instruction");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::LOADBOOL) == 2,
                "Current not-not lowering materializes a LOADBOOL pair");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::NOT) == 0,
                "Current not-not lowering avoids direct NOT opcodes");

}

void testLua51AssignmentRegisterReuseGapsAreCharacterized(TestSuite& suite) {
    Proto* localSwap = generateProto("local a,b = 1,2; a,b = b,a; return a,b");

    ASSERT_TRUE(suite, localSwap != nullptr, "Local swap proto generated");
    ASSERT_TRUE(suite, countOpcode(localSwap, OpCode::MOVE) <= 3,
                "Lua 5.1-style local swap assignment reuses one scratch register");
    ASSERT_TRUE(suite, hasReturn(localSwap, 0, 3),
                "Direct local swap return reuses contiguous registers after assignment");

    delete localSwap;

    Proto* tableAssign = generateProto("local t = {}; local v = 1; t.x = v; return t.x");

    ASSERT_TRUE(suite, tableAssign != nullptr, "Table assignment proto generated");
    ASSERT_TRUE(suite, countOpcode(tableAssign, OpCode::SETTABLE) == 1,
                "Table assignment still emits one SETTABLE instruction");
    ASSERT_TRUE(suite, countOpcode(tableAssign, OpCode::GETTABLE) == 1,
                "Table field return still emits one GETTABLE instruction");
    ASSERT_TRUE(suite, hasSetTableFromValueRegister(tableAssign, 0, 1),
                "Table assignment now uses original local table/value registers (no reuse gap)");

    delete tableAssign;
}

void testLua51ArithmeticConstantFoldingIsCharacterized(TestSuite& suite) {
    Proto* proto = generateProto("return 1 + 2 * 3");

    ASSERT_TRUE(suite, proto != nullptr, "Constant expression proto generated");
    ASSERT_TRUE(suite, proto->getConstantCount() == 1,
                "Constant expression folds to one numeric constant");
    if (proto->getConstantCount() == 1) {
        Value folded = proto->getConstant(0);
        ASSERT_TRUE(suite, folded.isNumber(), "Folded constant is numeric");
        if (folded.isNumber()) {
            ASSERT_EQ(suite, 7.0, folded.asNumber(), "Folded constant value is 7");
        }
    }
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::MUL) == 0,
                "Folded constant expression emits no MUL");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::ADD) == 0,
                "Folded constant expression emits no ADD");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::LOADK) == 1,
                "Folded constant expression materializes with one LOADK");

}

void testLua51SelfAssignmentElisionGapIsCharacterized(TestSuite& suite) {
    Proto* proto = generateProto("local a = ...; a = a; return a");

    ASSERT_TRUE(suite, proto != nullptr, "Self-assignment proto generated");
    ASSERT_TRUE(suite, countOpcode(proto, OpCode::MOVE) == 0,
                "Self-assignment elision plus direct local return leaves no MOVE");
    ASSERT_FALSE(suite, hasABC(proto, OpCode::MOVE, 1, 0, 0),
                 "Direct local return does not copy the local to a temporary");
    ASSERT_FALSE(suite, hasABC(proto, OpCode::MOVE, 0, 1, 0),
                 "Self-assignment does not write the copied value back");
    ASSERT_TRUE(suite, hasReturn(proto, 0, 2),
                "Self-assignment chunk returns directly from local register 0");

}

void registerCodegenCharacterizationTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Statement Lowering Runtime Keeps Loop And Scope Semantics",
                          testStatementLoweringRuntimeKeepsLoopAndScopeSemantics);
    registry.registerTest(kSuiteName, "Structured Statements Leave No Pending Jumps",
                          testStructuredStatementsLeaveNoPendingJumps);
    registry.registerTest(kSuiteName, "Generic For Bytecode Shape Is Stable",
                          testGenericForBytecodeShapeIsStable);
    registry.registerTest(kSuiteName, "Lua51 LOADNIL Merge And Direct Return Parity Gap Is Characterized",
                          testLua51LoadNilMergeAndDirectReturnParityGapIsCharacterized);
    registry.registerTest(kSuiteName, "Lua51 Nil Assignment In Loop Remains Observable",
                          testLua51NilAssignmentInLoopRemainsObservable);
    registry.registerTest(kSuiteName, "Lua51 Concat Merge Parity Gap Is Characterized",
                          testLua51ConcatMergeParityGapIsCharacterized);
    registry.registerTest(kSuiteName, "Lua51 Not Not Boolean Normalization Shape Is Characterized",
                          testLua51NotNotBooleanNormalizationShapeIsCharacterized);
    registry.registerTest(kSuiteName, "Lua51 Assignment Register Reuse Gaps Are Characterized",
                          testLua51AssignmentRegisterReuseGapsAreCharacterized);
    registry.registerTest(kSuiteName, "Lua51 Arithmetic Constant Folding Is Characterized",
                          testLua51ArithmeticConstantFoldingIsCharacterized);
    registry.registerTest(kSuiteName, "Lua51 Self Assignment Elision Gap Is Characterized",
                          testLua51SelfAssignmentElisionGapIsCharacterized);
}

