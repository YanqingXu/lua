/**
 * @file test_codegen_state.cpp
 * @brief Tests for the CodeGenerator state boundary.
 */

#include "../framework/test_framework.hpp"
#include "compiler/codegen_state.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "runtime/runtime_services.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Codegen State";

void testResetForProtoClearsTransientState(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    CodegenState state(services);

    state.pc = 12;
    state.currentLine = 34;
    state.locals.localVars_.emplace_back("old", 0, 0);
    state.locals.nactvar_ = 1;
    state.upvalues.add("captured", true, 0);
    state.blocks.enterBlock(true, 1);
    state.blocks.jpc_ = 9;
    state.regs.setFreeReg(5);

    Proto proto;
    state.resetForProto(proto, true, "state_test.lua");

    ASSERT_TRUE(suite, state.proto == &proto, "state should bind current proto");
    ASSERT_EQ(suite, 0, state.pc, "pc should reset");
    ASSERT_EQ(suite, 0, state.currentLine, "current line should reset");
    ASSERT_EQ(suite, 0, state.regs.current(), "register allocator should reset");
    ASSERT_TRUE(suite, state.locals.localVars_.empty(), "locals should clear");
    ASSERT_EQ(suite, 0, state.locals.nactvar_, "active locals should reset");
    ASSERT_EQ(suite, -1, state.upvalues.find("captured"), "upvalues should clear");
    ASSERT_TRUE(suite, state.blocks.currentBlock_ == nullptr, "block stack should clear");
    ASSERT_EQ(suite, NO_JUMP, state.blocks.jpc_, "pending jump list should reset");
    ASSERT_TRUE(suite, proto.isVararg(), "proto vararg flag should be set");
    ASSERT_EQ(suite, 2, static_cast<int>(proto.getMaxStackSize()), "proto minimum stack should be set");
    ASSERT_TRUE(suite, proto.getSource() != nullptr, "proto source should be set");
    ASSERT_EQ(suite, Str("state_test.lua"), proto.getSource()->getData(), "proto source should match");
}

}  // namespace

void registerCodegenStateTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Reset For Proto Clears Transient State", testResetForProtoClearsTransientState);
}
