/**
 * @file test_vm_dispatch.cpp
 * @brief Tests for VM opcode dispatch grouping.
 */

#include "../framework/test_framework.hpp"
#include "core/function.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm_dispatch.hpp"
#include "vm/vm_dispatch_strategy.hpp"
#include "vm/vm.hpp"

#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "VM Dispatch";

class RecordingDispatchStrategy final : public VM::DispatchStrategy {
public:
    ExecResult run(VM::VMContext& context) override {
        called = true;
        services = &context.services;
        state = context.state;
        proto = context.proto;
        nexeccalls = context.nexeccalls;
        return ExecResult::Returned;
    }

    const char* name() const noexcept override {
        return "recording";
    }

    bool called = false;
    RuntimeServices* services = nullptr;
    LuaState* state = nullptr;
    Proto* proto = nullptr;
    i32 nexeccalls = 0;
};

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

void testDefaultDispatchStrategyIsSwitch(TestSuite& suite) {
    ASSERT_TRUE(suite, std::string(VM::defaultDispatchStrategy().name()) == "switch",
                "Default dispatch strategy should be switch");
}

void testRuntimeServicesCanInjectDispatchStrategy(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    RecordingDispatchStrategy strategy;
    services.dispatchStrategy = &strategy;

    LuaState* L = LuaState::newState(services);
    Proto* proto = new Proto();
    proto->setMaxStackSize(1);
    Function* func = new Function(proto);
    func->setEnv(L->getGlobalTable());
    services.gc.registerObject(proto);
    services.gc.registerObject(func);

    VM::execute(services, L, func);

    ASSERT_TRUE(suite, strategy.called, "Injected dispatch strategy should run");
    ASSERT_TRUE(suite, strategy.services == &services, "Dispatch context carries RuntimeServices");
    ASSERT_TRUE(suite, strategy.state == L, "Dispatch context carries LuaState");
    ASSERT_TRUE(suite, strategy.proto == proto, "Dispatch context carries Proto");
    ASSERT_EQ(suite, 1, strategy.nexeccalls, "Dispatch context carries call depth");

    delete L;
    services.gc.clearAll();
}

}  // namespace

void registerVMDispatchTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Opcode Groups Cover Dispatch Families", testOpcodeGroupsCoverDispatchFamilies);
    registry.registerTest(kSuiteName, "All Opcodes Have Dispatch Group", testAllOpcodesHaveDispatchGroup);
    registry.registerTest(kSuiteName, "Metamethod Candidate Grouping", testMetamethodCandidateGrouping);
    registry.registerTest(kSuiteName, "Default Dispatch Strategy Is Switch", testDefaultDispatchStrategyIsSwitch);
    registry.registerTest(kSuiteName, "RuntimeServices Can Inject Dispatch Strategy",
                          testRuntimeServicesCanInjectDispatchStrategy);
}
