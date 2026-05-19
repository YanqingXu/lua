/**
 * @file test_bytecode_builder.cpp
 * @brief Tests for the CodeGenerator bytecode emission boundary.
 */

#include "../framework/test_framework.hpp"
#include "compiler/bytecode_builder.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Bytecode Builder";

void testEmitInstructionsTracksLineInfo(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    Proto proto;
    BytecodeBuilder builder;

    builder.bind(proto, pool);

    i32 loadBoolPc = builder.emitABC(21, OpCode::LOADBOOL, 1, 1, 0);
    i32 loadKPc = builder.emitABx(22, OpCode::LOADK, 2, 7);
    i32 jumpPc = builder.emitAsBx(23, OpCode::JMP, 0, -2);

    ASSERT_TRUE(suite, builder.isBound(), "builder should be bound");
    ASSERT_EQ(suite, 0, loadBoolPc, "first instruction pc");
    ASSERT_EQ(suite, 1, loadKPc, "second instruction pc");
    ASSERT_EQ(suite, 2, jumpPc, "third instruction pc");
    ASSERT_EQ(suite, 3, builder.instructionCount(), "instruction count should match");
    ASSERT_EQ(suite, 21, proto.getLine(loadBoolPc), "ABC line info should match");
    ASSERT_EQ(suite, 22, proto.getLine(loadKPc), "ABx line info should match");
    ASSERT_EQ(suite, 23, proto.getLine(jumpPc), "AsBx line info should match");
    ASSERT_EQ(suite, static_cast<int>(OpCode::LOADBOOL), static_cast<int>(GET_OPCODE(builder.instruction(loadBoolPc))),
              "ABC opcode should match");
    ASSERT_EQ(suite, 1, GETARG_A(builder.instruction(loadBoolPc)), "ABC A argument should match");
    ASSERT_EQ(suite, 1, GETARG_B(builder.instruction(loadBoolPc)), "ABC B argument should match");
    ASSERT_EQ(suite, 0, GETARG_C(builder.instruction(loadBoolPc)), "ABC C argument should match");
    ASSERT_EQ(suite, 7, GETARG_Bx(builder.instruction(loadKPc)), "ABx argument should match");
    ASSERT_EQ(suite, -2, GETARG_sBx(builder.instruction(jumpPc)), "AsBx argument should match");
    ASSERT_EQ(suite, static_cast<int>(OpCode::JMP), static_cast<int>(builder.lastOpcode()),
              "last opcode should match");
}

void testConstantsAndProtoWritesUseSharedBoundary(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    Proto proto;
    BytecodeBuilder builder;
    builder.bind(proto, pool);

    i32 firstString = builder.addStringConstant("shared");
    i32 secondString = builder.addStringConstant("shared");
    i32 number = builder.addNumberConstant(42.0);
    i32 boolean = builder.addBoolConstant(true);
    i32 nil = builder.addNilConstant();
    Proto child;
    i32 childIndex = builder.addSubProto(&child);

    ASSERT_EQ(suite, firstString, secondString, "string constants should dedupe through Proto");
    ASSERT_TRUE(suite, proto.getConstant(firstString).isString(), "string constant should be stored");
    ASSERT_EQ(suite, Str("shared"), proto.getConstant(firstString).asString()->getData(),
              "string constant should match");
    ASSERT_TRUE(suite, proto.getConstant(number).isNumber(), "number constant should be stored");
    ASSERT_EQ(suite, 42.0, proto.getConstant(number).asNumber(), "number constant should match");
    ASSERT_TRUE(suite, proto.getConstant(boolean).isBoolean(), "bool constant should be stored");
    ASSERT_TRUE(suite, proto.getConstant(boolean).asBoolean(), "bool constant should match");
    ASSERT_TRUE(suite, proto.getConstant(nil).isNil(), "nil constant should be stored");
    ASSERT_EQ(suite, 0, childIndex, "child proto index should start at zero");
    ASSERT_TRUE(suite, proto.getSubProto(childIndex) == &child, "child proto should be stored");
}

void testInstructionReplacementUsesSharedBoundary(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    Proto proto;
    BytecodeBuilder builder;
    builder.bind(proto, pool);

    i32 pc = builder.emitABC(7, OpCode::MOVE, 0, 1, 0);
    Instruction inst = builder.instruction(pc);
    SETARG_A(inst, 4);
    builder.replaceInstruction(pc, inst);

    ASSERT_EQ(suite, 4, GETARG_A(proto.getInstruction(pc)), "replacement should update instruction");
    ASSERT_EQ(suite, 7, proto.getLine(pc), "replacement should preserve line info");
}

}  // namespace

void registerBytecodeBuilderTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Emit Instructions Tracks Line Info", testEmitInstructionsTracksLineInfo);
    registry.registerTest(kSuiteName, "Constants And Proto Writes Use Shared Boundary",
                          testConstantsAndProtoWritesUseSharedBoundary);
    registry.registerTest(kSuiteName, "Instruction Replacement Uses Shared Boundary",
                          testInstructionReplacementUsesSharedBoundary);
}
