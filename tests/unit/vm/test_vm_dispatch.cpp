/**
 * @file test_vm_dispatch.cpp
 * @brief Tests for VM opcode dispatch grouping.
 */

#include "../framework/test_framework.hpp"
#include "vm/vm_dispatch.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "VM Dispatch";

void testOpcodeGroupsCoverDispatchFamilies(TestSuite& suite) {
    ASSERT_TRUE(suite, VM::isDataMoveOpcode(OpCode::MOVE), "MOVE is data move");
    ASSERT_TRUE(suite, VM::isDataMoveOpcode(OpCode::LOADK), "LOADK is data move");
    ASSERT_EQ(suite, static_cast<int>(VM::OpcodeGroup::Global), static_cast<int>(VM::opcodeGroup(OpCode::GETGLOBAL)),
              "GETGLOBAL is global");
    ASSERT_EQ(suite, static_cast<int>(VM::OpcodeGroup::Upvalue), static_cast<int>(VM::opcodeGroup(OpCode::SETUPVAL)),
              "SETUPVAL is upvalue");
    ASSERT_TRUE(suite, VM::isTableOpcode(OpCode::GETTABLE), "GETTABLE is table");
    ASSERT_TRUE(suite, VM::isTableOpcode(OpCode::SELF), "SELF is table");
    ASSERT_TRUE(suite, VM::isArithmeticOpcode(OpCode::ADD), "ADD is arithmetic");
    ASSERT_TRUE(suite, VM::isArithmeticOpcode(OpCode::POW), "POW is arithmetic");
    ASSERT_EQ(suite, static_cast<int>(VM::OpcodeGroup::Unary), static_cast<int>(VM::opcodeGroup(OpCode::LEN)),
              "LEN is unary");
    ASSERT_TRUE(suite, VM::isComparisonOpcode(OpCode::EQ), "EQ is comparison");
    ASSERT_TRUE(suite, VM::isComparisonOpcode(OpCode::LE), "LE is comparison");
    ASSERT_EQ(suite, static_cast<int>(VM::OpcodeGroup::Branch), static_cast<int>(VM::opcodeGroup(OpCode::TESTSET)),
              "TESTSET is branch");
    ASSERT_TRUE(suite, VM::isCallOpcode(OpCode::CALL), "CALL is call");
    ASSERT_TRUE(suite, VM::isCallOpcode(OpCode::RETURN), "RETURN is call");
    ASSERT_EQ(suite, static_cast<int>(VM::OpcodeGroup::Loop), static_cast<int>(VM::opcodeGroup(OpCode::FORLOOP)),
              "FORLOOP is loop");
    ASSERT_EQ(suite, static_cast<int>(VM::OpcodeGroup::Closure), static_cast<int>(VM::opcodeGroup(OpCode::CLOSURE)),
              "CLOSURE is closure");
    ASSERT_EQ(suite, static_cast<int>(VM::OpcodeGroup::Vararg), static_cast<int>(VM::opcodeGroup(OpCode::VARARG)),
              "VARARG is vararg");
}

void testAllOpcodesHaveDispatchGroup(TestSuite& suite) {
    for (i32 index = 0; index < NUM_OPCODES; ++index) {
        OpCode op = static_cast<OpCode>(index);
        ASSERT_TRUE(suite, VM::opcodeGroup(op) != VM::OpcodeGroup::Unknown, "opcode should have dispatch group");
    }
}

void testMetamethodCandidateGrouping(TestSuite& suite) {
    ASSERT_TRUE(suite, VM::mayInvokeMetamethod(OpCode::GETTABLE), "GETTABLE may invoke metamethod");
    ASSERT_TRUE(suite, VM::mayInvokeMetamethod(OpCode::SETTABLE), "SETTABLE may invoke metamethod");
    ASSERT_TRUE(suite, VM::mayInvokeMetamethod(OpCode::ADD), "ADD may invoke metamethod");
    ASSERT_TRUE(suite, VM::mayInvokeMetamethod(OpCode::CONCAT), "CONCAT may invoke metamethod");
    ASSERT_TRUE(suite, VM::mayInvokeMetamethod(OpCode::CALL), "CALL may invoke __call");
    ASSERT_FALSE(suite, VM::mayInvokeMetamethod(OpCode::MOVE), "MOVE does not invoke metamethod");
    ASSERT_FALSE(suite, VM::mayInvokeMetamethod(OpCode::RETURN), "RETURN does not invoke metamethod");
}

}  // namespace

void registerVMDispatchTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Opcode Groups Cover Dispatch Families", testOpcodeGroupsCoverDispatchFamilies);
    registry.registerTest(kSuiteName, "All Opcodes Have Dispatch Group", testAllOpcodesHaveDispatchGroup);
    registry.registerTest(kSuiteName, "Metamethod Candidate Grouping", testMetamethodCandidateGrouping);
}
