/**
 * @file test_vm_dispatch.cpp
 * @brief Tests for VM opcode dispatch grouping.
 */

#include "../framework/test_framework.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm_dispatch.hpp"
#include "vm/vm_dispatch_strategy.hpp"
#include "vm/vm_handlers.hpp"
#include "vm/vm_switch_dispatch.hpp"
#include "vm/vm.hpp"

#include <array>
#include <string>
#include <utility>

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

i32 pr17GenericForIterator(LuaState* L) {
    if (L->getTop() < 2 || !L->at(1).isNumber() || !L->at(2).isNumber()) {
        L->pushNil();
        return 1;
    }

    f64 limit = L->at(1).asNumber();
    f64 control = L->at(2).asNumber();
    f64 next = control + 1.0;
    if (next > limit) {
        L->pushNil();
        return 1;
    }

    L->pushNumber(next);
    return 1;
}

i32 pr19ReturnArgPlusFive(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isNumber()) {
        L->pushNil();
        return 1;
    }

    L->pushNumber(L->at(1).asNumber() + 5.0);
    return 1;
}

i32 pr19YieldingFunction(LuaState* L) {
    L->setStatus(ThreadStatus::Yield);
    L->setYieldResults(0);
    return 0;
}

Proto* compileDispatchChunk(RuntimeServices& services, const char* source, const char* sourceName) {
    Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }

    Chunk chunk = std::move(*parsed);
    CodeGenerator codegen(services);
    return codegen.generate(chunk, sourceName);
}

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

void testTableDispatchStrategyIsAvailable(TestSuite& suite) {
    VM::DispatchStrategy& table = VM::tableDispatchStrategy();

    ASSERT_TRUE(suite, std::string(table.name()) == "table",
                "Table dispatch strategy should identify itself");
    ASSERT_TRUE(suite, &table != &VM::defaultDispatchStrategy(),
                "Table dispatch strategy should be distinct from the default switch strategy");
}

void testTableDispatchExecutesCompiledChunk(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    VM::DispatchStrategy& table = VM::tableDispatchStrategy();
    services.dispatchStrategy = &table;

    Proto* proto = compileDispatchChunk(services, R"(
        function pr20_pair(a, b)
            if a < b then
                return a + b, a * b
            end
            return 0, 0
        end

        function pr20_relay()
            return pr20_pair(6, 7)
        end

        local x, y = pr20_relay()
        return x, y
    )", "=(table_dispatch_chunk)");

    LuaState* L = LuaState::newState(services);
    Function* func = new Function(proto);
    func->setEnv(L->getGlobalTable());
    services.gc.registerObject(func);

    VM::execute(services, L, func);

    ASSERT_TRUE(suite, L->getTop() >= 2, "Table dispatch should return two values");
    ASSERT_TRUE(suite, L->at(-2).isNumber() && L->at(-2).asNumber() == 13.0,
                "Table dispatch should execute arithmetic and return the sum");
    ASSERT_TRUE(suite, L->at(-1).isNumber() && L->at(-1).asNumber() == 42.0,
                "Table dispatch should execute tailcalls and return the product");

    delete L;
    services.gc.clearAll();
}

void testSwitchDispatchHelpersCoverOpcodeSpace(TestSuite& suite) {
    using SwitchOpHandler = VM::detail::SwitchOpHandler;

    const std::array<std::pair<OpCode, SwitchOpHandler>, static_cast<usize>(NUM_OPCODES)> expected = {{
        {OpCode::MOVE, VM::detail::execOpMove},
        {OpCode::LOADK, VM::detail::execOpLoadK},
        {OpCode::LOADBOOL, VM::detail::execOpLoadBool},
        {OpCode::LOADNIL, VM::detail::execOpLoadNil},
        {OpCode::GETGLOBAL, VM::detail::execOpGetGlobal},
        {OpCode::SETGLOBAL, VM::detail::execOpSetGlobal},
        {OpCode::GETUPVAL, VM::detail::execOpGetUpval},
        {OpCode::SETUPVAL, VM::detail::execOpSetUpval},
        {OpCode::GETTABLE, VM::detail::execOpGetTable},
        {OpCode::SETTABLE, VM::detail::execOpSetTable},
        {OpCode::NEWTABLE, VM::detail::execOpNewTable},
        {OpCode::SELF, VM::detail::execOpSelf},
        {OpCode::ADD, VM::detail::execOpAdd},
        {OpCode::SUB, VM::detail::execOpSub},
        {OpCode::MUL, VM::detail::execOpMul},
        {OpCode::DIV, VM::detail::execOpDiv},
        {OpCode::MOD, VM::detail::execOpMod},
        {OpCode::POW, VM::detail::execOpPow},
        {OpCode::UNM, VM::detail::execOpUnm},
        {OpCode::NOT, VM::detail::execOpNot},
        {OpCode::LEN, VM::detail::execOpLen},
        {OpCode::CONCAT, VM::detail::execOpConcat},
        {OpCode::JMP, VM::detail::execOpJmp},
        {OpCode::EQ, VM::detail::execOpEq},
        {OpCode::LT, VM::detail::execOpLt},
        {OpCode::LE, VM::detail::execOpLe},
        {OpCode::TEST, VM::detail::execOpTest},
        {OpCode::TESTSET, VM::detail::execOpTestSet},
        {OpCode::CALL, VM::detail::execOpCall},
        {OpCode::TAILCALL, VM::detail::execOpTailCall},
        {OpCode::RETURN, VM::detail::execOpReturn},
        {OpCode::FORLOOP, VM::detail::execOpForLoop},
        {OpCode::FORPREP, VM::detail::execOpForPrep},
        {OpCode::TFORLOOP, VM::detail::execOpTForLoop},
        {OpCode::SETLIST, VM::detail::execOpSetList},
        {OpCode::CLOSE, VM::detail::execOpClose},
        {OpCode::CLOSURE, VM::detail::execOpClosure},
        {OpCode::VARARG, VM::detail::execOpVararg},
    }};

    for (usize i = 0; i < expected.size(); ++i) {
        const OpCode opcode = expected[i].first;
        ASSERT_TRUE(suite, expected[i].second != nullptr, "switch helper should be callable");
        ASSERT_TRUE(suite, VM::detail::switchHandlerFor(opcode) == expected[i].second,
                    "switch handler lookup should return the opcode-specific helper");
    }
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
        ASSERT_EQ(suite, static_cast<int>(opcodeMetadata(expected).group), static_cast<int>(entry.group),
                  "Handler entry group should match opcode metadata");
    }
}

void testHandlersCoverMigratedOpcodes(TestSuite& suite) {
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::MOVE), "MOVE should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::LOADK), "LOADK should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::LOADBOOL), "LOADBOOL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::LOADNIL), "LOADNIL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::GETGLOBAL), "GETGLOBAL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::SETGLOBAL), "SETGLOBAL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::GETUPVAL), "GETUPVAL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::SETUPVAL), "SETUPVAL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::GETTABLE), "GETTABLE should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::SETTABLE), "SETTABLE should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::NEWTABLE), "NEWTABLE should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::SELF), "SELF should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::SETLIST), "SETLIST should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::ADD), "ADD should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::SUB), "SUB should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::MUL), "MUL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::DIV), "DIV should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::MOD), "MOD should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::POW), "POW should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::UNM), "UNM should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::NOT), "NOT should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::LEN), "LEN should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::CONCAT), "CONCAT should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::JMP), "JMP should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::EQ), "EQ should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::LT), "LT should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::LE), "LE should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::TEST), "TEST should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::TESTSET), "TESTSET should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::CLOSE), "CLOSE should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::FORLOOP), "FORLOOP should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::FORPREP), "FORPREP should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::TFORLOOP), "TFORLOOP should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::CLOSURE), "CLOSURE should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::VARARG), "VARARG should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::CALL), "CALL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::TAILCALL), "TAILCALL should have a handler");
    ASSERT_TRUE(suite, VM::hasHandler(OpCode::RETURN), "RETURN should have a handler");
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

void testGlobalAndUpvalueHandlersExecuteDirectly(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    Proto proto;
    GCString* globalName = services.strings.intern("pr12_global");
    usize globalNameIndex = proto.addConstant(Value(globalName));

    Function func(&proto);
    func.setEnv(L->getGlobalTable());

    Upvalue* upvalue = Upvalue::createClosed(Value(12.0));
    services.gc.registerObject(upvalue);
    func.addUpvalue(upvalue);

    Stack& stack = L->getStack();
    usize frameBase = L->getCurrentCallInfo().base;
    while (stack.size() < frameBase + 4) {
        stack.push(Value());
    }
    L->setAbsoluteTop(frameBase + 4);

    Value* base = &stack[frameBase];
    usize pc = 0;
    VM::OpExecutionContext context{
        services,
        L,
        &func,
        &proto,
        base,
        pc,
        0,
        1
    };

    L->getGlobalTable()->set(Value(globalName), Value(24.0));
    VM::runHandler(context, CREATE_ABx(OpCode::GETGLOBAL, 0, static_cast<i32>(globalNameIndex)));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 24.0,
                "GETGLOBAL handler should read from the function environment");

    base[1] = Value(36.0);
    VM::runHandler(context, CREATE_ABx(OpCode::SETGLOBAL, 1, static_cast<i32>(globalNameIndex)));
    Value storedGlobal = L->getGlobalTable()->get(Value(globalName));
    ASSERT_TRUE(suite, storedGlobal.isNumber() && storedGlobal.asNumber() == 36.0,
                "SETGLOBAL handler should write to the function environment");

    VM::runHandler(context, CREATE_ABC(OpCode::GETUPVAL, 2, 0, 0));
    base = context.base;
    ASSERT_TRUE(suite, base[2].isNumber() && base[2].asNumber() == 12.0,
                "GETUPVAL handler should read the selected upvalue");

    base[3] = Value(48.0);
    VM::runHandler(context, CREATE_ABC(OpCode::SETUPVAL, 3, 0, 0));
    Value storedUpvalue = upvalue->getValue(L->getStack());
    ASSERT_TRUE(suite, storedUpvalue.isNumber() && storedUpvalue.asNumber() == 48.0,
                "SETUPVAL handler should write the selected upvalue");

    delete L;
    services.gc.clearAll();
}

void testTableHandlersExecuteDirectly(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    Proto proto;
    GCString* fieldName = services.strings.intern("pr13_field");
    usize fieldNameIndex = proto.addConstant(Value(fieldName));

    Table* table = new Table();
    services.gc.registerObject(table);

    Stack& stack = L->getStack();
    usize frameBase = L->getCurrentCallInfo().base;
    while (stack.size() < frameBase + 8) {
        stack.push(Value());
    }
    L->setAbsoluteTop(frameBase + 8);

    Value* base = &stack[frameBase];
    usize pc = 0;
    VM::OpExecutionContext context{
        services,
        L,
        nullptr,
        &proto,
        base,
        pc,
        0,
        1
    };

    table->set(Value(fieldName), Value(55.0));
    base[1] = Value(table);
    VM::runHandler(context, CREATE_ABC(OpCode::GETTABLE, 0, 1, RKASK(static_cast<i32>(fieldNameIndex))));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 55.0,
                "GETTABLE handler should read table fields through RK keys");

    base[2] = Value(table);
    base[3] = Value(fieldName);
    base[4] = Value(66.0);
    VM::runHandler(context, CREATE_ABC(OpCode::SETTABLE, 2, 3, 4));
    Value storedField = table->get(Value(fieldName));
    ASSERT_TRUE(suite, storedField.isNumber() && storedField.asNumber() == 66.0,
                "SETTABLE handler should write table fields through register keys");

    VM::runHandler(context, CREATE_ABC(OpCode::NEWTABLE, 5, 0, 0));
    base = context.base;
    ASSERT_TRUE(suite, base[5].isTable(), "NEWTABLE handler should create a table");

    table->set(Value(fieldName), Value(77.0));
    base[6] = Value(table);
    VM::runHandler(context, CREATE_ABC(OpCode::SELF, 0, 6, RKASK(static_cast<i32>(fieldNameIndex))));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 77.0,
                "SELF handler should load the selected method field");
    ASSERT_TRUE(suite, base[1].isTable() && base[1].asTable() == table,
                "SELF handler should copy the receiver to A+1");

    Table* list = base[5].asTable();
    base[6] = Value(100.0);
    base[7] = Value(200.0);
    VM::runHandler(context, CREATE_ABC(OpCode::SETLIST, 5, 2, 1));
    Value first = list->getArray(1);
    Value second = list->getArray(2);
    ASSERT_TRUE(suite, first.isNumber() && first.asNumber() == 100.0,
                "SETLIST handler should write the first array element");
    ASSERT_TRUE(suite, second.isNumber() && second.asNumber() == 200.0,
                "SETLIST handler should write the second array element");

    delete L;
    services.gc.clearAll();
}

void testArithmeticHandlersExecuteDirectly(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    Proto proto;
    usize constantIndex = proto.addConstant(Value(3.0));

    Stack& stack = L->getStack();
    usize frameBase = L->getCurrentCallInfo().base;
    while (stack.size() < frameBase + 3) {
        stack.push(Value());
    }
    L->setAbsoluteTop(frameBase + 3);

    Value* base = &stack[frameBase];
    usize pc = 0;
    VM::OpExecutionContext context{
        services,
        L,
        nullptr,
        &proto,
        base,
        pc,
        0,
        1
    };

    base[1] = Value(10.0);
    base[2] = Value(2.0);
    VM::runHandler(context, CREATE_ABC(OpCode::ADD, 0, 1, RKASK(static_cast<i32>(constantIndex))));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 13.0,
                "ADD handler should support RK constants");

    VM::runHandler(context, CREATE_ABC(OpCode::SUB, 0, 1, 2));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 8.0,
                "SUB handler should subtract register operands");

    VM::runHandler(context, CREATE_ABC(OpCode::MUL, 0, 1, 2));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 20.0,
                "MUL handler should multiply register operands");

    VM::runHandler(context, CREATE_ABC(OpCode::DIV, 0, 1, 2));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 5.0,
                "DIV handler should divide register operands");

    VM::runHandler(context, CREATE_ABC(OpCode::MOD, 0, 1, RKASK(static_cast<i32>(constantIndex))));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 1.0,
                "MOD handler should support RK constants");

    base[1] = Value(2.0);
    VM::runHandler(context, CREATE_ABC(OpCode::POW, 0, 1, RKASK(static_cast<i32>(constantIndex))));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 8.0,
                "POW handler should support RK constants");

    delete L;
    services.gc.clearAll();
}

void testUnaryHandlersExecuteDirectly(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    Proto proto;
    GCString* hello = services.strings.intern("hello");
    GCString* spaceWorld = services.strings.intern(" world");

    Stack& stack = L->getStack();
    usize frameBase = L->getCurrentCallInfo().base;
    while (stack.size() < frameBase + 4) {
        stack.push(Value());
    }
    L->setAbsoluteTop(frameBase + 4);

    Value* base = &stack[frameBase];
    usize pc = 0;
    VM::OpExecutionContext context{
        services,
        L,
        nullptr,
        &proto,
        base,
        pc,
        0,
        1
    };

    base[1] = Value(7.0);
    VM::runHandler(context, CREATE_ABC(OpCode::UNM, 0, 1, 0));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == -7.0,
                "UNM handler should negate numeric operands");

    base[1] = Value(false);
    VM::runHandler(context, CREATE_ABC(OpCode::NOT, 0, 1, 0));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isBoolean() && base[0].asBoolean(),
                "NOT handler should invert Lua truthiness");

    base[1] = Value(hello);
    VM::runHandler(context, CREATE_ABC(OpCode::LEN, 0, 1, 0));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 5.0,
                "LEN handler should measure strings");

    base[1] = Value(hello);
    base[2] = Value(spaceWorld);
    VM::runHandler(context, CREATE_ABC(OpCode::CONCAT, 0, 1, 2));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isString() && std::string(base[0].asString()->c_str()) == "hello world",
                "CONCAT handler should concatenate string operands");

    delete L;
    services.gc.clearAll();
}

void testBranchAndComparisonHandlersExecuteDirectly(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    Proto proto;
    usize eightIndex = proto.addConstant(Value(8.0));
    proto.addInstruction(CREATE_ABC(OpCode::MOVE, 0, 0, 0));
    proto.addInstruction(CREATE_AsBx(OpCode::JMP, 0, 2));

    Stack& stack = L->getStack();
    usize frameBase = L->getCurrentCallInfo().base;
    while (stack.size() < frameBase + 4) {
        stack.push(Value());
    }
    L->setAbsoluteTop(frameBase + 4);

    Value* base = &stack[frameBase];
    usize pc = 0;
    VM::OpExecutionContext context{
        services,
        L,
        nullptr,
        &proto,
        base,
        pc,
        0,
        1
    };

    pc = 5;
    VM::runHandler(context, CREATE_AsBx(OpCode::JMP, 0, -2));
    ASSERT_EQ(suite, 3, static_cast<int>(pc), "JMP handler should apply sBx to pc");

    base[1] = Value(4.0);
    base[2] = Value(4.0);
    pc = 10;
    VM::runHandler(context, CREATE_ABC(OpCode::EQ, 0, 1, 2));
    base = context.base;
    ASSERT_EQ(suite, 11, static_cast<int>(pc), "EQ handler should skip when result differs from A");

    base[1] = Value(3.0);
    base[2] = Value(7.0);
    pc = 20;
    VM::runHandler(context, CREATE_ABC(OpCode::LT, 0, 1, 2));
    base = context.base;
    ASSERT_EQ(suite, 21, static_cast<int>(pc), "LT handler should skip when result differs from A");

    base[1] = Value(8.0);
    pc = 30;
    VM::runHandler(context, CREATE_ABC(OpCode::LE, 1, 1, RKASK(static_cast<i32>(eightIndex))));
    base = context.base;
    ASSERT_EQ(suite, 30, static_cast<int>(pc), "LE handler should keep pc when result matches A");

    base[0] = Value(false);
    pc = 1;
    VM::runHandler(context, CREATE_ABC(OpCode::TEST, 0, 0, 0));
    ASSERT_EQ(suite, 4, static_cast<int>(pc), "TEST handler should apply next JMP when condition matches");

    base[0] = Value(true);
    pc = 1;
    VM::runHandler(context, CREATE_ABC(OpCode::TEST, 0, 0, 0));
    ASSERT_EQ(suite, 2, static_cast<int>(pc), "TEST handler should advance once when condition does not match");

    base[0] = Value(99.0);
    base[2] = Value(42.0);
    pc = 1;
    VM::runHandler(context, CREATE_ABC(OpCode::TESTSET, 0, 2, 1));
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 42.0,
                "TESTSET handler should copy B when condition matches");
    ASSERT_EQ(suite, 4, static_cast<int>(pc), "TESTSET handler should apply next JMP when condition matches");

    base[0] = Value(99.0);
    base[2] = Value(42.0);
    pc = 1;
    VM::runHandler(context, CREATE_ABC(OpCode::TESTSET, 0, 2, 0));
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 99.0,
                "TESTSET handler should not copy B when condition does not match");
    ASSERT_EQ(suite, 2, static_cast<int>(pc), "TESTSET handler should advance once when condition does not match");

    delete L;
    services.gc.clearAll();
}

void testLoopAndCloseHandlersExecuteDirectly(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    Proto proto;
    proto.addInstruction(CREATE_AsBx(OpCode::JMP, 0, 2));
    Function iterator(pr17GenericForIterator);

    Stack& stack = L->getStack();
    CallInfo& ci = L->getCurrentCallInfo();
    usize frameBase = ci.base;
    while (stack.size() < frameBase + 12) {
        stack.push(Value());
    }
    ci.top = frameBase + 12;
    L->setAbsoluteTop(frameBase + 12);

    Value* base = &stack[frameBase];
    usize pc = 0;
    VM::OpExecutionContext context{
        services,
        L,
        nullptr,
        &proto,
        base,
        pc,
        0,
        1
    };

    base[0] = Value(10.0);
    base[1] = Value(20.0);
    Upvalue* lower = L->findOrCreateUpvalue(frameBase);
    Upvalue* upper = L->findOrCreateUpvalue(frameBase + 1);
    VM::runHandler(context, CREATE_ABC(OpCode::CLOSE, 1, 0, 0));
    ASSERT_TRUE(suite, lower->isOpen(), "CLOSE handler should leave lower upvalues open");
    ASSERT_TRUE(suite, upper->isClosed(), "CLOSE handler should close upvalues at base plus A");
    ASSERT_TRUE(suite, upper->getValue(stack).isNumber() && upper->getValue(stack).asNumber() == 20.0,
                "CLOSE handler should preserve the closed value");

    base[0] = Value(1.0);
    base[1] = Value(3.0);
    base[2] = Value(1.0);
    pc = 5;
    VM::runHandler(context, CREATE_AsBx(OpCode::FORPREP, 0, 4));
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 0.0,
                "FORPREP handler should initialize index to init minus step");
    ASSERT_EQ(suite, 9, static_cast<int>(pc), "FORPREP handler should apply sBx to pc");

    base[0] = Value(0.0);
    base[1] = Value(3.0);
    base[2] = Value(1.0);
    base[3] = Value();
    pc = 10;
    VM::runHandler(context, CREATE_AsBx(OpCode::FORLOOP, 0, -2));
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 1.0,
                "FORLOOP handler should update the internal index when continuing");
    ASSERT_TRUE(suite, base[3].isNumber() && base[3].asNumber() == 1.0,
                "FORLOOP handler should update the visible loop variable when continuing");
    ASSERT_EQ(suite, 8, static_cast<int>(pc), "FORLOOP handler should jump when continuing");

    base[0] = Value(3.0);
    base[1] = Value(3.0);
    base[2] = Value(1.0);
    base[3] = Value(99.0);
    pc = 20;
    VM::runHandler(context, CREATE_AsBx(OpCode::FORLOOP, 0, -2));
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 3.0,
                "FORLOOP handler should leave the internal index when terminating");
    ASSERT_TRUE(suite, base[3].isNumber() && base[3].asNumber() == 99.0,
                "FORLOOP handler should leave the visible loop variable when terminating");
    ASSERT_EQ(suite, 20, static_cast<int>(pc), "FORLOOP handler should not jump when terminating");

    base[0] = Value(&iterator);
    base[1] = Value(2.0);
    base[2] = Value(0.0);
    pc = 0;
    VM::runHandler(context, CREATE_ABC(OpCode::TFORLOOP, 0, 0, 1));
    base = context.base;
    ASSERT_TRUE(suite, base[2].isNumber() && base[2].asNumber() == 1.0,
                "TFORLOOP handler should store the non-nil iterator result as control");
    ASSERT_EQ(suite, 3, static_cast<int>(pc), "TFORLOOP handler should apply the following JMP when continuing");

    base[0] = Value(&iterator);
    base[1] = Value(1.0);
    base[2] = Value(1.0);
    pc = 0;
    VM::runHandler(context, CREATE_ABC(OpCode::TFORLOOP, 0, 0, 1));
    base = context.base;
    ASSERT_TRUE(suite, base[2].isNumber() && base[2].asNumber() == 1.0,
                "TFORLOOP handler should keep control when iterator returns nil");
    ASSERT_EQ(suite, 1, static_cast<int>(pc), "TFORLOOP handler should advance once when iterator returns nil");

    delete L;
    services.gc.clearAll();
}

void testClosureAndVarargHandlersExecuteDirectly(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    Proto parentProto;
    Proto childProto;
    childProto.setNumUpvalues(2);
    usize childIndex = parentProto.addProto(&childProto);
    parentProto.addInstruction(CREATE_ABC(OpCode::MOVE, 0, 2, 0));
    parentProto.addInstruction(CREATE_ABC(OpCode::GETUPVAL, 0, 0, 0));

    Function parentFunc(&parentProto);
    Upvalue* parentUpvalue = Upvalue::createClosed(Value(456.0));
    services.gc.registerObject(parentUpvalue);
    parentFunc.addUpvalue(parentUpvalue);

    Stack& stack = L->getStack();
    CallInfo& ci = L->getCurrentCallInfo();
    ci.func = 0;
    ci.base = 4;
    ci.top = 12;
    while (stack.size() < ci.top) {
        stack.push(Value());
    }
    L->setAbsoluteTop(ci.top);

    Value* base = &stack[ci.base];
    base[2] = Value(123.0);
    usize pc = 0;
    VM::OpExecutionContext context{
        services,
        L,
        &parentFunc,
        &parentProto,
        base,
        pc,
        0,
        1
    };

    VM::runHandler(context, CREATE_ABx(OpCode::CLOSURE, 0, static_cast<i32>(childIndex)));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isFunction(), "CLOSURE handler should create a function");
    Function* closure = base[0].asFunction();
    ASSERT_TRUE(suite, closure->getProto() == &childProto, "CLOSURE handler should use selected child proto");
    ASSERT_EQ(suite, 2, static_cast<int>(closure->getUpvalueCount()),
              "CLOSURE handler should attach child upvalues");
    ASSERT_EQ(suite, 2, static_cast<int>(pc), "CLOSURE handler should consume upvalue pseudo instructions");

    Upvalue* localCapture = closure->getUpvalue(0);
    ASSERT_TRUE(suite, localCapture != nullptr && localCapture->isOpen(),
                "CLOSURE handler should capture MOVE upvalues as open locals");
    ASSERT_TRUE(suite, localCapture->getValue(stack).isNumber() &&
                       localCapture->getValue(stack).asNumber() == 123.0,
                "CLOSURE handler should capture the frame-relative local value");
    ASSERT_TRUE(suite, closure->getUpvalue(1) == parentUpvalue,
                "CLOSURE handler should reuse parent GETUPVAL captures");

    Proto varargProto;
    varargProto.setNumParams(1);
    varargProto.setVararg(true);
    context.proto = &varargProto;
    context.function = nullptr;

    stack[2] = Value(11.0);
    stack[3] = Value(22.0);
    base = &stack[ci.base];
    context.base = base;
    VM::runHandler(context, CREATE_ABC(OpCode::VARARG, 0, 3, 0));
    base = context.base;
    ASSERT_TRUE(suite, base[0].isNumber() && base[0].asNumber() == 11.0,
                "VARARG handler should copy the first fixed requested vararg");
    ASSERT_TRUE(suite, base[1].isNumber() && base[1].asNumber() == 22.0,
                "VARARG handler should copy the second fixed requested vararg");

    base[2] = Value();
    base[3] = Value();
    VM::runHandler(context, CREATE_ABC(OpCode::VARARG, 2, 0, 0));
    base = context.base;
    ASSERT_TRUE(suite, base[2].isNumber() && base[2].asNumber() == 11.0,
                "VARARG handler should copy the first open requested vararg");
    ASSERT_TRUE(suite, base[3].isNumber() && base[3].asNumber() == 22.0,
                "VARARG handler should copy the second open requested vararg");
    ASSERT_EQ(suite, static_cast<int>(ci.base + 4), static_cast<int>(L->getAbsoluteTop()),
              "VARARG handler should extend absolute top for open results");

    delete L;
    services.gc.clearAll();
}

void testCallAndReturnHandlersExecuteDirectly(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();

    {
        LuaState* L = LuaState::newState(services);

        Proto callerProto;
        callerProto.setMaxStackSize(8);
        callerProto.addInstruction(CREATE_ABC(OpCode::CALL, 0, 2, 2));
        callerProto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));

        Function callerFunc(&callerProto);
        Function cFunc(pr19ReturnArgPlusFive);

        Stack& stack = L->getStack();
        CallInfo& ci = L->getCurrentCallInfo();
        ci.func = 0;
        ci.base = 1;
        ci.top = 9;
        ci.nresults = 0;
        while (stack.size() < ci.top) {
            stack.push(Value());
        }

        stack[ci.func] = Value(&callerFunc);
        stack[ci.base] = Value(&cFunc);
        stack[ci.base + 1] = Value(37.0);
        L->setAbsoluteTop(ci.base + 2);

        Value* base = &stack[ci.base];
        usize pc = 1;
        VM::OpExecutionContext context{
            services,
            L,
            &callerFunc,
            &callerProto,
            base,
            pc,
            0,
            1
        };

        VM::HandlerStatus status = VM::runHandler(context, CREATE_ABC(OpCode::CALL, 0, 2, 2));
        ASSERT_EQ(suite, static_cast<int>(VM::HandlerStatus::Continue), static_cast<int>(status),
                  "CALL handler should continue after a C call returns normally");
        ASSERT_TRUE(suite, stack[ci.base].isNumber() && stack[ci.base].asNumber() == 42.0,
                    "CALL handler should store fixed C call results at R(A)");
        ASSERT_EQ(suite, 1, static_cast<int>(L->getCallStackSize()),
                  "CALL handler should pop the C CallInfo after a normal C return");
        ASSERT_EQ(suite, static_cast<int>(ci.top), static_cast<int>(L->getAbsoluteTop()),
                  "CALL handler should restore the caller fixed-result top");

        delete L;
    }

    {
        LuaState* L = LuaState::newState(services);

        Proto callerProto;
        callerProto.setMaxStackSize(8);
        callerProto.addInstruction(CREATE_ABC(OpCode::CALL, 0, 1, 1));
        callerProto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));

        Proto calleeProto;
        calleeProto.setMaxStackSize(3);
        calleeProto.setNumParams(0);

        Function callerFunc(&callerProto);
        Function calleeFunc(&calleeProto);

        Stack& stack = L->getStack();
        CallInfo& callerCI = L->getCurrentCallInfo();
        callerCI.func = 0;
        callerCI.base = 1;
        callerCI.top = 9;
        callerCI.nresults = 0;
        while (stack.size() < callerCI.top) {
            stack.push(Value());
        }

        stack[callerCI.func] = Value(&callerFunc);
        stack[callerCI.base] = Value(&calleeFunc);
        L->setAbsoluteTop(callerCI.base + 1);

        Value* base = &stack[callerCI.base];
        usize pc = 1;
        VM::OpExecutionContext context{
            services,
            L,
            &callerFunc,
            &callerProto,
            base,
            pc,
            0,
            1
        };

        VM::HandlerStatus status = VM::runHandler(context, CREATE_ABC(OpCode::CALL, 0, 1, 1));
        ASSERT_EQ(suite, static_cast<int>(VM::HandlerStatus::Reenter), static_cast<int>(status),
                  "CALL handler should request reentry after a Lua call");
        ASSERT_EQ(suite, 2, context.nexeccalls, "CALL handler should increment Lua call depth");
        ASSERT_EQ(suite, 2, static_cast<int>(L->getCallStackSize()),
                  "CALL handler should push a Lua CallInfo");
        ASSERT_TRUE(suite, L->getCallStack()[0].savedpc == callerProto.getCode().data() + pc,
                    "CALL handler should save the caller pc before entering Lua");
        const CallInfo& calleeCI = L->getCurrentCallInfo();
        ASSERT_EQ(suite, static_cast<int>(callerCI.base), static_cast<int>(calleeCI.func),
                  "CALL handler should place the callee function at the new frame function slot");

        delete L;
    }

    {
        LuaState* L = LuaState::newState(services);

        Proto callerProto;
        callerProto.setMaxStackSize(4);
        callerProto.addInstruction(CREATE_ABC(OpCode::CALL, 0, 1, 1));
        callerProto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));

        Function callerFunc(&callerProto);
        Function yieldingFunc(pr19YieldingFunction);

        Stack& stack = L->getStack();
        CallInfo& ci = L->getCurrentCallInfo();
        ci.func = 0;
        ci.base = 1;
        ci.top = 6;
        ci.nresults = 0;
        while (stack.size() < ci.top) {
            stack.push(Value());
        }

        stack[ci.func] = Value(&callerFunc);
        stack[ci.base] = Value(&yieldingFunc);
        L->setAbsoluteTop(ci.base + 1);

        Value* base = &stack[ci.base];
        usize pc = 1;
        VM::OpExecutionContext context{
            services,
            L,
            &callerFunc,
            &callerProto,
            base,
            pc,
            0,
            3
        };

        VM::HandlerStatus status = VM::runHandler(context, CREATE_ABC(OpCode::CALL, 0, 1, 1));
        ASSERT_EQ(suite, static_cast<int>(VM::HandlerStatus::Yielded), static_cast<int>(status),
                  "CALL handler should surface yielded C calls");
        ASSERT_EQ(suite, 3, L->getSavedNexeccalls(),
                  "CALL handler should save call depth when yielding");

        delete L;
    }

    {
        LuaState* L = LuaState::newState(services);

        Proto callerProto;
        callerProto.setMaxStackSize(8);
        callerProto.addInstruction(CREATE_ABC(OpCode::TAILCALL, 0, 1, 0));
        callerProto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 0, 0));

        Proto calleeProto;
        calleeProto.setMaxStackSize(4);

        Function callerFunc(&callerProto);
        Function calleeFunc(&calleeProto);

        Stack& stack = L->getStack();
        CallInfo& ci = L->getCurrentCallInfo();
        ci.func = 0;
        ci.base = 1;
        ci.top = 9;
        ci.tailcalls = 2;
        while (stack.size() < ci.top) {
            stack.push(Value());
        }

        stack[ci.func] = Value(&callerFunc);
        stack[ci.base] = Value(&calleeFunc);
        L->setAbsoluteTop(ci.base + 1);

        Value* base = &stack[ci.base];
        usize pc = 1;
        VM::OpExecutionContext context{
            services,
            L,
            &callerFunc,
            &callerProto,
            base,
            pc,
            0,
            4
        };

        VM::HandlerStatus status = VM::runHandler(context, CREATE_ABC(OpCode::TAILCALL, 0, 1, 0));
        ASSERT_EQ(suite, static_cast<int>(VM::HandlerStatus::Reenter), static_cast<int>(status),
                  "TAILCALL handler should request reentry for Lua tail calls");
        ASSERT_EQ(suite, 1, static_cast<int>(L->getCallStackSize()),
                  "TAILCALL handler should reuse the current CallInfo");
        const CallInfo& reusedCI = L->getCurrentCallInfo();
        ASSERT_EQ(suite, 0, static_cast<int>(reusedCI.func),
                  "TAILCALL handler should move the callee into the caller function slot");
        ASSERT_EQ(suite, 3, reusedCI.tailcalls,
                  "TAILCALL handler should increment the reused frame tailcall count");
        ASSERT_TRUE(suite, stack[reusedCI.func].isFunction() && stack[reusedCI.func].asFunction() == &calleeFunc,
                    "TAILCALL handler should preserve the callee function in the reused frame");

        delete L;
    }

    {
        LuaState* L = LuaState::newState(services);

        Proto proto;
        proto.setMaxStackSize(4);

        Function func(&proto);

        Stack& stack = L->getStack();
        CallInfo& ci = L->getCurrentCallInfo();
        ci.func = 0;
        ci.base = 1;
        ci.top = 5;
        ci.nresults = 0;
        while (stack.size() < ci.top) {
            stack.push(Value());
        }

        stack[ci.func] = Value(&func);
        stack[ci.base] = Value(64.0);
        L->setAbsoluteTop(ci.base + 1);

        Value* base = &stack[ci.base];
        usize pc = 0;
        VM::OpExecutionContext context{
            services,
            L,
            &func,
            &proto,
            base,
            pc,
            0,
            1
        };

        VM::HandlerStatus status = VM::runHandler(context, CREATE_ABC(OpCode::RETURN, 0, 2, 0));
        ASSERT_EQ(suite, static_cast<int>(VM::HandlerStatus::Returned), static_cast<int>(status),
                  "RETURN handler should report outermost frame completion");
        ASSERT_EQ(suite, 0, context.nexeccalls, "RETURN handler should decrement call depth");
        ASSERT_TRUE(suite, stack[0].isNumber() && stack[0].asNumber() == 64.0,
                    "RETURN handler should move returned values to ci.func");
        ASSERT_EQ(suite, 1, static_cast<int>(L->getAbsoluteTop()),
                  "RETURN handler should shrink absolute top to the returned values");

        delete L;
    }

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
    registry.registerTest(kSuiteName, "Table Dispatch Strategy Is Available",
                          testTableDispatchStrategyIsAvailable);
    registry.registerTest(kSuiteName, "Table Dispatch Executes Compiled Chunk",
                          testTableDispatchExecutesCompiledChunk);
    registry.registerTest(kSuiteName, "Switch Dispatch Helpers Cover Opcode Space",
                          testSwitchDispatchHelpersCoverOpcodeSpace);
    registry.registerTest(kSuiteName, "Handler Table Covers Opcode Space", testHandlerTableCoversOpcodeSpace);
    registry.registerTest(kSuiteName, "Handlers Cover Migrated Opcodes",
                          testHandlersCoverMigratedOpcodes);
    registry.registerTest(kSuiteName, "Data Move Handlers Execute Directly", testDataMoveHandlersExecuteDirectly);
    registry.registerTest(kSuiteName, "Global And Upvalue Handlers Execute Directly",
                          testGlobalAndUpvalueHandlersExecuteDirectly);
    registry.registerTest(kSuiteName, "Table Handlers Execute Directly",
                          testTableHandlersExecuteDirectly);
    registry.registerTest(kSuiteName, "Arithmetic Handlers Execute Directly",
                          testArithmeticHandlersExecuteDirectly);
    registry.registerTest(kSuiteName, "Unary Handlers Execute Directly",
                          testUnaryHandlersExecuteDirectly);
    registry.registerTest(kSuiteName, "Branch And Comparison Handlers Execute Directly",
                          testBranchAndComparisonHandlersExecuteDirectly);
    registry.registerTest(kSuiteName, "Loop And Close Handlers Execute Directly",
                          testLoopAndCloseHandlersExecuteDirectly);
    registry.registerTest(kSuiteName, "Closure And Vararg Handlers Execute Directly",
                          testClosureAndVarargHandlersExecuteDirectly);
    registry.registerTest(kSuiteName, "Call And Return Handlers Execute Directly",
                          testCallAndReturnHandlersExecuteDirectly);
}
