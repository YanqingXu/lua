/**
 * @file test_vm_trace_debug.cpp
 * @brief Regression tests for VM trace and debug hook event boundaries.
 */

#include "../framework/test_framework.hpp"

#include "compiler/codegen.hpp"
#include "compiler/parser.hpp"
#include "core/function.hpp"
#include "debug/trace_sink.hpp"
#include "debug/trace_types.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm.hpp"

#include <string>
#include <vector>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "VM Trace Debug";

struct RecordingTraceSink final : ITraceSink {
    Vec<TraceEvent> events;

    void onInstruction(const TraceEvent& evt) override { events.push_back(evt); }
    void onCall(const TraceEvent& evt) override { events.push_back(evt); }
    void onReturn(const TraceEvent& evt) override { events.push_back(evt); }
    void onError(const TraceEvent& evt) override { events.push_back(evt); }
    void flush() override {}
};

struct HookRecord {
    Str event;
    i32 line = -1;
};

Vec<HookRecord>* g_hookRecords = nullptr;

Proto* compileChunk(RuntimeServices& services, const char* source, const char* sourceName) {
    Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(services);
    return codegen.generate(chunk, sourceName);
}

bool runLuaChunk(RuntimeServices& services, LuaState* L, const char* source,
                 const char* sourceName, RecordingTraceSink* traceSink = nullptr) {
    Proto* proto = nullptr;

    try {
        proto = compileChunk(services, source, sourceName);
        if (proto == nullptr) {
            return false;
        }

        Function* func = new Function(proto);
        services.gc.registerObject(func);
        func->setEnv(L->getGlobalTable());

        VM::setTraceSink(traceSink);
        VM::execute(services, L, func);
        VM::setTraceSink(nullptr);

        delete proto;
        return true;
    } catch (...) {
        VM::setTraceSink(nullptr);
        delete proto;
        return false;
    }
}

usize firstTraceIndex(const RecordingTraceSink& sink, TraceEventKind kind) {
    for (usize index = 0; index < sink.events.size(); ++index) {
        if (sink.events[index].kind == kind) {
            return index;
        }
    }

    return sink.events.size();
}

usize firstHookIndex(const Vec<HookRecord>& records, const Str& event) {
    for (usize index = 0; index < records.size(); ++index) {
        if (records[index].event == event) {
            return index;
        }
    }

    return records.size();
}

i32 captureDebugHook(LuaState* L) {
    if (g_hookRecords == nullptr) {
        return 0;
    }

    HookRecord record;
    record.event = L->isString(1) ? L->toString(1) : "";
    if (!L->isNil(2) && L->isNumber(2)) {
        record.line = static_cast<i32>(L->toNumber(2));
    }

    g_hookRecords->push_back(record);
    return 0;
}

}  // namespace

void testTraceEventsKeepInstructionCallReturnOrder(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);
    RecordingTraceSink sink;

    bool ok = runLuaChunk(
        services,
        L,
        "local function plus_one(x)\n"
        "    return x + 1\n"
        "end\n"
        "local result = plus_one(41)\n"
        "return result\n",
        "test_vm_trace_order.lua",
        &sink);

    ASSERT_TRUE(suite, ok, "trace target chunk runs");
    ASSERT_TRUE(suite, !sink.events.empty(), "trace sink captured events");

    usize firstInstruction = firstTraceIndex(sink, TraceEventKind::Instruction);
    usize firstCall = firstTraceIndex(sink, TraceEventKind::Call);
    usize firstReturn = firstTraceIndex(sink, TraceEventKind::Return);

    ASSERT_EQ(suite, static_cast<usize>(0), firstInstruction, "trace starts with instruction event");
    ASSERT_TRUE(suite, firstCall < sink.events.size(), "trace captures call event");
    ASSERT_TRUE(suite, firstReturn < sink.events.size(), "trace captures return event");
    ASSERT_TRUE(suite, firstInstruction < firstCall, "instruction event precedes call event");
    ASSERT_TRUE(suite, firstCall < firstReturn, "call event precedes return event");

    for (usize index = 0; index < sink.events.size(); ++index) {
        ASSERT_EQ(suite, static_cast<u64>(index), sink.events[index].seq, "trace sequence is monotonic");
    }

    if (firstCall < sink.events.size()) {
        ASSERT_TRUE(suite, sink.events[firstCall].callDepth > 0, "call trace records call depth");
    }

    delete L;
}

void testDebugHooksKeepCountLineCallReturnOrder(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    Vec<HookRecord> records;
    g_hookRecords = &records;

    Function* hookFunc = new Function(captureDebugHook);
    services.gc.registerObject(hookFunc);
    L->setDebugHook(hookFunc, HookMaskCall | HookMaskReturn | HookMaskLine, 1);

    bool ok = runLuaChunk(
        services,
        L,
        "local function target()\n"
        "    local x = 1\n"
        "    x = x + 1\n"
        "    return x\n"
        "end\n"
        "return target()\n",
        "test_vm_debug_hook_order.lua");

    L->setDebugHook(nullptr, 0, 0);
    g_hookRecords = nullptr;

    ASSERT_TRUE(suite, ok, "debug hook target chunk runs");
    ASSERT_TRUE(suite, !records.empty(), "debug hook captured events");

    usize firstCall = firstHookIndex(records, "call");
    usize firstCount = firstHookIndex(records, "count");
    usize firstLine = firstHookIndex(records, "line");
    usize firstReturn = firstHookIndex(records, "return");

    ASSERT_TRUE(suite, firstCall < records.size(), "debug hook captures call event");
    ASSERT_TRUE(suite, firstCount < records.size(), "debug hook captures count event");
    ASSERT_TRUE(suite, firstLine < records.size(), "debug hook captures line event");
    ASSERT_TRUE(suite, firstReturn < records.size(), "debug hook captures return event");
    ASSERT_TRUE(suite, firstCall < firstCount, "call hook precedes instruction count hook");
    ASSERT_TRUE(suite, firstCount < firstLine, "count hook precedes line hook for first instruction");
    ASSERT_TRUE(suite, firstLine < firstReturn, "line hook precedes return hook");

    if (firstLine < records.size()) {
        ASSERT_TRUE(suite, records[firstLine].line > 0, "line hook includes source line");
    }

    delete L;
}

void registerVMTraceDebugTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Trace Events Keep Instruction Call Return Order",
                          testTraceEventsKeepInstructionCallReturnOrder);
    registry.registerTest(kSuiteName, "Debug Hooks Keep Count Line Call Return Order",
                          testDebugHooksKeepCountLineCallReturnOrder);
}
