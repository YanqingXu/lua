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
#include "vm/vm_handlers.hpp"
#include "vm/vm.hpp"

#include <array>
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

void testHandlerTableCoversOpcodeSpace(TestSuite& suite) {
    const auto& table = VM::handlerTable();

    ASSERT_EQ(suite, NUM_OPCODES, static_cast<int>(table.size()),
              "Handler table should have one entry per opcode");

    for (i32 index = 0; index < NUM_OPCODES; ++index) {
        const VM::HandlerEntry& entry = table[static_cast<usize>(index)];
        OpCode expected = static_cast<OpCode>(index);

        ASSERT_EQ(suite, index, static_cast<int>(entry.opcode),
                  "Handler entry opcode should match table index");
        ASSERT_TRUE(suite, std::string(entry.name) == getOpName(expected),
                    "Handler entry name should match opcode name");
    }
}

void testInitialHandlersCoverDataMoveOpcodes(TestSuite& suite) {
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::MOVE), "MOVE should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::LOADK), "LOADK should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::LOADBOOL), "LOADBOOL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::LOADNIL), "LOADNIL should have a handler");
    ASSERT_FALSE(suite, VM::hasHandler(OpCode::CALL), "CALL remains switch-only in the first handler-table slice");
}

void testDataMoveHandlersExecuteDirectly(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    Proto proto;
    usize constantIndex = proto.addConstant(Value(42.0));

    std::array<Value, 4> registers{};
    registers[1] = Value(7.0);
    registers[3] = Value(99.0);

    Value* base = registers.data();
    usize pc = 10;
    VM::OpExecutionContext context{
        services,
        nullptr,
        nullptr,
        &proto,
        base,
        pc,
        0,
        1
    };

    VM::HandlerStatus moveStatus = VM::runHandler(context, CREATE_ABC(OpCode::MOVE, 0, 1, 0));
    ASSERT_EQ(suite, static_cast<int>(VM::HandlerStatus::Continue), static_cast<int>(moveStatus),
              "MOVE handler should continue dispatch");
    ASSERT_TRUE(suite, registers[0].isNumber() && registers[0].asNumber() == 7.0,
                "MOVE handler should copy source register");

    VM::runHandler(context, CREATE_ABx(OpCode::LOADK, 2, static_cast<i32>(constantIndex)));
    ASSERT_TRUE(suite, registers[2].isNumber() && registers[2].asNumber() == 42.0,
                "LOADK handler should load a constant");

    pc = 5;
    VM::runHandler(context, CREATE_ABC(OpCode::LOADBOOL, 0, 1, 1));
    ASSERT_TRUE(suite, registers[0].isBoolean() && registers[0].asBoolean(),
                "LOADBOOL handler should write a boolean");
    ASSERT_EQ(suite, 6, static_cast<int>(pc), "LOADBOOL handler should skip the next instruction when C is set");

    VM::runHandler(context, CREATE_ABC(OpCode::LOADNIL, 1, 3, 0));
    ASSERT_TRUE(suite, registers[1].isNil(), "LOADNIL handler should clear first register");
    ASSERT_TRUE(suite, registers[2].isNil(), "LOADNIL handler should clear middle register");
    ASSERT_TRUE(suite, registers[3].isNil(), "LOADNIL handler should clear last register");
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
    registry.registerTest(kSuiteName, "Handler Table Covers Opcode Space", testHandlerTableCoversOpcodeSpace);
    registry.registerTest(kSuiteName, "Initial Handlers Cover Data Move Opcodes",
                          testInitialHandlersCoverDataMoveOpcodes);
    registry.registerTest(kSuiteName, "Data Move Handlers Execute Directly", testDataMoveHandlersExecuteDirectly);
}
