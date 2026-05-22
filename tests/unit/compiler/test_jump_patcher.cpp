/**
 * @file test_jump_patcher.cpp
 * @brief Tests for the CodeGenerator jump patching boundary.
 */

#include "../framework/test_framework.hpp"
#include "compiler/codegen/jump_patcher.hpp"
#include "compiler/codegen/codegen_state.hpp"
#include "core/function.hpp"
#include "runtime/runtime_services.hpp"

#include <stdexcept>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Jump Patcher";

struct JumpFixture {
    RuntimeServices services = RuntimeServices::fromSingletons();
    CodegenState state{services};
    Proto proto;

    JumpFixture() {
        state.resetForProto(proto, true, "jump_patcher_test");
    }
};

i32 jumpTarget(const JumpPatcher& patcher, i32 pc) {
    return patcher.getJump(pc);
}

} // namespace

void testPendingJumpListFlushesToCurrentPc(TestSuite& suite) {
    JumpFixture fixture;
    JumpPatcher patcher(fixture.state);

    i32 jumpPc = patcher.emitJump();
    patcher.patchToHere(jumpPc);
    patcher.flushPendingJumps();

    ASSERT_EQ(suite, 1, fixture.state.bytecode.instructionCount(), "flush should not emit instructions");
    ASSERT_EQ(suite, 1, jumpTarget(patcher, jumpPc), "pending jump should target current pc");
    ASSERT_EQ(suite, NO_JUMP, fixture.state.blocks.jpc_, "pending jump list should be drained");
}

void testJumpKeepsNewJumpAsListHead(TestSuite& suite) {
    JumpFixture fixture;
    JumpPatcher patcher(fixture.state);

    i32 firstJump = patcher.emitJump();
    patcher.patchToHere(firstJump);
    i32 secondJump = patcher.emitJump();

    ASSERT_EQ(suite, 2, fixture.state.bytecode.instructionCount(), "new jump should be emitted");
    ASSERT_EQ(suite, firstJump, jumpTarget(patcher, secondJump), "new jump should link to old pending list");
    ASSERT_EQ(suite, NO_JUMP, jumpTarget(patcher, firstJump), "old pending tail should remain unresolved");
    ASSERT_EQ(suite, NO_JUMP, fixture.state.blocks.jpc_, "pending list should be drained");
}

void testPatchListWritesExplicitTargets(TestSuite& suite) {
    JumpFixture fixture;
    JumpPatcher patcher(fixture.state);

    i32 firstJump = patcher.emitJump();
    i32 secondJump = patcher.emitJump();

    PatchList list;
    list.append(firstJump);
    list.append(secondJump);

    patcher.patchList(list, 7);

    ASSERT_EQ(suite, 7, jumpTarget(patcher, firstJump), "first explicit patch target");
    ASSERT_EQ(suite, 7, jumpTarget(patcher, secondJump), "second explicit patch target");
}

void testConditionalJumpLowersNoRegTestSetToTest(TestSuite& suite) {
    JumpFixture fixture;
    JumpPatcher patcher(fixture.state);

    i32 jumpPc = patcher.emitConditionalJump(OpCode::TESTSET, NO_REG, 3, 1);
    Instruction testInst = fixture.state.bytecode.instruction(0);

    ASSERT_EQ(suite, 2, fixture.state.bytecode.instructionCount(), "conditional jump emits test and jmp");
    ASSERT_EQ(suite, static_cast<int>(OpCode::TEST), static_cast<int>(GET_OPCODE(testInst)),
              "TESTSET with NO_REG should lower to TEST");
    ASSERT_EQ(suite, 3, GETARG_A(testInst), "TEST A should come from original B");
    ASSERT_EQ(suite, 0, GETARG_B(testInst), "TEST B should be cleared");
    ASSERT_EQ(suite, 1, GETARG_C(testInst), "TEST C should be preserved");
    ASSERT_EQ(suite, NO_JUMP, jumpTarget(patcher, jumpPc), "conditional jump should start unresolved");
}

void testFixJumpRejectsOutOfRangeTargets(TestSuite& suite) {
    JumpFixture fixture;
    JumpPatcher patcher(fixture.state);
    i32 jumpPc = patcher.emitJump();

    bool threw = false;
    try {
        patcher.fixJump(jumpPc, MAXARG_sBx + 2);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    ASSERT_TRUE(suite, threw, "fixJump should reject control structures that are too long");
}

void registerJumpPatcherTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Pending Jump List Flushes To Current Pc",
                          testPendingJumpListFlushesToCurrentPc);
    registry.registerTest(kSuiteName, "Jump Keeps New Jump As List Head", testJumpKeepsNewJumpAsListHead);
    registry.registerTest(kSuiteName, "PatchList Writes Explicit Targets", testPatchListWritesExplicitTargets);
    registry.registerTest(kSuiteName, "Conditional Jump Lowers NoReg TestSet To Test",
                          testConditionalJumpLowersNoRegTestSetToTest);
    registry.registerTest(kSuiteName, "FixJump Rejects Out Of Range Targets",
                          testFixJumpRejectsOutOfRangeTargets);
}
