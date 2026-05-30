/**
 * @file test_scope_manager.cpp
 * @brief Tests for the CodeGenerator scope management boundary.
 */

#include "../framework/test_framework.hpp"
#include "compiler/codegen/jump_patcher.hpp"
#include "compiler/codegen/scope_manager.hpp"
#include "compiler/codegen/codegen_state.hpp"
#include "core/function.hpp"
#include "runtime/runtime_services.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Scope Manager";

struct ScopeFixture {
    RuntimeServices services = RuntimeServices::fromSingletons();
    CodegenState state{services};
    Proto proto;
    JumpPatcher jumps{state};
    ScopeManager scopes{state, jumps};

    ScopeFixture() {
        state.resetForProto(proto, true, "scope_manager_test");
    }
};

}  // namespace

void testLocalLifecycleClosesScopeAndResetsRegisters(TestSuite& suite) {
    ScopeFixture fixture;

    i32 reg = fixture.scopes.addLocalVar("captured");
    fixture.scopes.adjustLocalVars(1);
    fixture.scopes.markLocalCaptured(reg);
    fixture.scopes.removeLocalVars(0);

    ASSERT_EQ(suite, 0, reg, "first local should use register zero");
    ASSERT_EQ(suite, -1, fixture.scopes.findLocalVar("captured"), "removed local should no longer resolve");
    ASSERT_EQ(suite, 0, fixture.scopes.activeLocalCount(), "active locals should reset");
    ASSERT_EQ(suite, 0, fixture.state.registers.current(), "register cursor should reset to active locals");
    ASSERT_EQ(suite, 1, fixture.state.bytecode.instructionCount(), "scope close should emit CLOSE");

    Instruction closeInst = fixture.state.bytecode.instruction(0);
    ASSERT_EQ(suite, static_cast<int>(OpCode::CLOSE), static_cast<int>(GET_OPCODE(closeInst)),
              "removeLocalVars should emit CLOSE before closing locals");
    ASSERT_EQ(suite, 0, GETARG_A(closeInst), "CLOSE should target the removed scope level");
    ASSERT_EQ(suite, 1, fixture.scopes.localVars().front().endpc, "closed local endpc should follow CLOSE");
}

void testReturnSuppressesRedundantClose(TestSuite& suite) {
    ScopeFixture fixture;

    fixture.scopes.addLocalVar("value");
    fixture.scopes.adjustLocalVars(1);
    fixture.state.bytecode.emitABC(12, OpCode::RETURN, 0, 1, 0);
    fixture.scopes.removeLocalVars(0);

    ASSERT_EQ(suite, 1, fixture.state.bytecode.instructionCount(), "RETURN should suppress redundant CLOSE");
    ASSERT_EQ(suite, static_cast<int>(OpCode::RETURN),
              static_cast<int>(GET_OPCODE(fixture.state.bytecode.instruction(0))),
              "existing RETURN should remain the final instruction");
    ASSERT_EQ(suite, 0, fixture.scopes.activeLocalCount(), "locals should still be removed after RETURN");
}

void testBreakableBlockPatchesBreakListOnLeave(TestSuite& suite) {
    ScopeFixture fixture;

    fixture.scopes.enterBlock(true);
    BlockInfo* block = fixture.scopes.findBreakableBlock();
    ASSERT_TRUE(suite, block != nullptr, "breakable block should be discoverable");
    if (block == nullptr) {
        return;
    }

    i32 breakJump = fixture.jumps.emitJump();
    fixture.scopes.appendBreakJump(*block, breakJump);
    fixture.scopes.leaveBlock();

    ASSERT_TRUE(suite, fixture.scopes.currentBlock() == nullptr, "leaving block should restore previous block");
    ASSERT_EQ(suite, NO_JUMP, fixture.state.blockManager.jpc_, "break jump should not leak into pending jpc");
    ASSERT_EQ(suite, 1, fixture.jumps.getJump(breakJump), "break jump should target current pc after leave");
}

void testUpvalueContextDeduplicatesCaptures(TestSuite& suite) {
    ScopeFixture fixture;

    i32 first = fixture.scopes.addUpvalue("outer", true, 2);
    i32 second = fixture.scopes.addUpvalue("outer", false, 7);

    ASSERT_EQ(suite, 0, first, "first upvalue should use index zero");
    ASSERT_EQ(suite, first, second, "same upvalue name should deduplicate");
    ASSERT_EQ(suite, first, fixture.scopes.findUpvalue("outer"), "findUpvalue should return existing index");
    ASSERT_EQ(suite, -1, fixture.scopes.findUpvalue("missing"), "missing upvalue should return -1");
    ASSERT_EQ(suite, 1, static_cast<int>(fixture.scopes.upvalues().size()), "only one capture should be stored");
}

void registerScopeManagerTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Local Lifecycle Closes Scope And Resets Registers",
                          testLocalLifecycleClosesScopeAndResetsRegisters);
    registry.registerTest(kSuiteName, "Return Suppresses Redundant Close", testReturnSuppressesRedundantClose);
    registry.registerTest(kSuiteName, "Breakable Block Patches Break List On Leave",
                          testBreakableBlockPatchesBreakListOnLeave);
    registry.registerTest(kSuiteName, "Upvalue Context Deduplicates Captures",
                          testUpvalueContextDeduplicatesCaptures);
}
