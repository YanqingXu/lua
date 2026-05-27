/**
 * @file test_vm_trace_debug.cpp
 * @brief Regression tests for VM trace and debug hook event boundaries.
 */

#include "../framework/test_framework.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "debug/json_trace_sink.hpp"
#include "debug/trace_sink.hpp"
#include "debug/trace_types.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <filesystem>
#include <fstream>
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
                 const char* sourceName, ITraceSink* traceSink = nullptr, bool traceDiff = false) {
    Proto* proto = nullptr;

    try {
        proto = compileChunk(services, source, sourceName);
        if (proto == nullptr) {
            return false;
        }

        Function* func = new Function(proto);
        services.gc.registerObject(func);
        func->setEnv(L->getGlobalTable());

        VM::setTraceDiffEnabled(traceDiff);
        VM::setTraceSink(traceSink);
        VM::execute(services, L, func);
        VM::setTraceSink(nullptr);
        VM::setTraceDiffEnabled(false);

        delete proto;
        return true;
    } catch (...) {
        VM::setTraceSink(nullptr);
        VM::setTraceDiffEnabled(false);
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

Str readTextFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return Str(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );
}

Str normalizeLineEndings(Str text) {
    usize pos = 0;
    while ((pos = text.find("\r\n", pos)) != Str::npos) {
        text.replace(pos, 2, "\n");
        pos += 1;
    }
    return text;
}

Str writeTraceGoldenJsonl(const std::filesystem::path& path,
                          const char* sourceName,
                          bool traceDiff) {
    std::filesystem::remove(path);

    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    bool ok = false;
    {
        JsonTraceSink sink(path.string(), 64);
        ok = sink.isOpen()
          && runLuaChunk(
                 services,
                 L,
                 "local x = 1\n"
                 "x = x + 2\n"
                 "return x\n",
                 sourceName,
                 &sink,
                 traceDiff);
        sink.flush();
    }

    delete L;
    if (!ok) {
        return "";
    }

    return normalizeLineEndings(readTextFile(path));
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

    for (const TraceEvent& event : sink.events) {
        ASSERT_TRUE(suite, !event.funcName.empty(), "trace event records a function name");
        ASSERT_TRUE(suite, event.funcName != "?", "trace event function name is resolved");
    }

    ASSERT_EQ(suite, Str("test_vm_trace_order.lua"), sink.events[firstInstruction].funcName,
              "instruction trace uses current proto name");
    ASSERT_EQ(suite, Str("test_vm_trace_order.lua:1"), sink.events[firstCall].funcName,
              "call trace uses callee proto name");
    ASSERT_EQ(suite, Str("test_vm_trace_order.lua:1"), sink.events[firstReturn].funcName,
              "return trace uses returning proto name");

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

void testJsonTraceSinkWritesStableJsonLines(TestSuite& suite) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "lua_cpp_trace_json_sink_test.jsonl";
    std::filesystem::remove(path);

    {
        JsonTraceSink sink(path.string(), 4);
        ASSERT_TRUE(suite, sink.isOpen(), "json trace sink opens temp file");

        TraceEvent instruction;
        instruction.seq = 7;
        instruction.kind = TraceEventKind::Instruction;
        instruction.pc = 3;
        instruction.op = OpCode::LOADK;
        instruction.a = 1;
        instruction.b = 2;
        instruction.c = 3;
        instruction.bx = 4;
        instruction.sbx = -5;
        instruction.line = 99;
        instruction.source = "trace \"src\"\n.lua";
        instruction.callDepth = 2;
        instruction.funcName = "trace\"fn";
        instruction.includeChangedRegisters = true;
        TraceRegisterChange change;
        change.slot = 1;
        change.hasName = true;
        change.name = "x";
        change.oldValue = "null";
        change.newValue = "42";
        change.oldType = "nil";
        change.newType = "number";
        instruction.changedRegisters.push_back(change);
        sink.onInstruction(instruction);

        TraceEvent call;
        call.seq = 8;
        call.kind = TraceEventKind::Call;
        call.funcName = "fn\"name";
        call.source = "call.lua";
        call.line = 12;
        call.callDepth = 3;
        sink.onCall(call);

        TraceEvent ret;
        ret.seq = 9;
        ret.kind = TraceEventKind::Return;
        ret.funcName = "ret\"fn";
        ret.source = "return.lua";
        ret.line = 13;
        ret.callDepth = 2;
        sink.onReturn(ret);

        TraceEvent error;
        error.seq = 10;
        error.kind = TraceEventKind::Error;
        error.funcName = "err\"fn";
        error.errorMsg = "bad\tthing";
        error.source = "err.lua";
        error.line = 14;
        error.callDepth = 1;
        sink.onError(error);

        sink.flush();
        ASSERT_EQ(suite, static_cast<u64>(4), sink.getEventCount(), "json trace sink records four events");
    }

    const Str content = readTextFile(path);
    ASSERT_TRUE(suite, content.find(
        "{\"seq\":7,\"kind\":\"instruction\",\"funcName\":\"trace\\\"fn\",\"pc\":3,\"op\":\"LOADK\","
        "\"a\":1,\"b\":2,\"c\":3,\"bx\":4,\"sbx\":-5,\"line\":99,"
        "\"source\":\"trace \\\"src\\\"\\n.lua\",\"callDepth\":2,"
        "\"changedRegisters\":[{\"slot\":1,\"name\":\"x\",\"old\":null,"
        "\"new\":42,\"oldType\":\"nil\",\"newType\":\"number\"}]}"
    ) != Str::npos, "instruction trace JSON line is stable");
    ASSERT_TRUE(suite, content.find(
        "{\"seq\":8,\"kind\":\"call\",\"funcName\":\"fn\\\"name\","
        "\"source\":\"call.lua\",\"line\":12,\"callDepth\":3}"
    ) != Str::npos, "call trace JSON line is stable");
    ASSERT_TRUE(
        suite,
        content.find(
            "{\"seq\":9,\"kind\":\"return\",\"funcName\":\"ret\\\"fn\","
            "\"source\":\"return.lua\",\"line\":13,\"callDepth\":2}"
        ) != Str::npos,
        "return trace JSON line is stable"
    );
    ASSERT_TRUE(suite, content.find(
        "{\"seq\":10,\"kind\":\"error\",\"funcName\":\"err\\\"fn\",\"message\":\"bad\\tthing\","
        "\"source\":\"err.lua\",\"line\":14,\"callDepth\":1}"
    ) != Str::npos, "error trace JSON line is stable");

    std::filesystem::remove(path);
}

void testTraceDiffWritesChangedRegisters(TestSuite& suite) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "lua_cpp_trace_diff_test.jsonl";
    std::filesystem::remove(path);

    RuntimeServices services = RuntimeServices::fromSingletons();
    LuaState* L = LuaState::newState(services);

    bool ok = false;
    {
        JsonTraceSink sink(path.string(), 64);
        ASSERT_TRUE(suite, sink.isOpen(), "json trace diff sink opens temp file");

        ok = runLuaChunk(
            services,
            L,
            "local x = 1\n"
            "x = x + 2\n"
            "return x\n",
            "test_vm_trace_diff.lua",
            &sink,
            true);
        sink.flush();
    }

    ASSERT_TRUE(suite, ok, "trace diff target chunk runs");

    const Str content = readTextFile(path);
    ASSERT_TRUE(suite, content.find("\"changedRegisters\"") != Str::npos,
                "trace diff writes changedRegisters");
    ASSERT_TRUE(suite, content.find("\"funcName\":\"test_vm_trace_diff.lua\"") != Str::npos,
                "trace diff writes instruction function names");
    ASSERT_TRUE(suite, content.find("\"registers\"") == Str::npos,
                "trace diff omits full register snapshots");
    ASSERT_TRUE(suite, content.find("\"old\":null") != Str::npos,
                "trace diff records old nil value");
    ASSERT_TRUE(suite, content.find("\"new\":1") != Str::npos,
                "trace diff records first assigned value");
    ASSERT_TRUE(suite, content.find("\"new\":3") != Str::npos,
                "trace diff records updated value");

    std::filesystem::remove(path);
    delete L;
}

void testTraceJsonlPlainGolden(TestSuite& suite) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "lua_cpp_trace_plain_golden_test.jsonl";

    const Str actual = writeTraceGoldenJsonl(path, "trace_plain_golden.lua", false);
    const Str expected =
        "{\"seq\":0,\"kind\":\"instruction\",\"funcName\":\"trace_plain_golden.lua\",\"pc\":0,\"op\":\"LOADK\",\"a\":0,\"b\":0,\"c\":0,\"bx\":0,\"sbx\":-131071,\"line\":1,\"source\":\"trace_plain_golden.lua\",\"callDepth\":1,\"registers\":[{\"slot\":0,\"name\":null,\"value\":null,\"type\":\"nil\"},{\"slot\":1,\"name\":null,\"value\":null,\"type\":\"nil\"}]}\n"
        "{\"seq\":1,\"kind\":\"instruction\",\"funcName\":\"trace_plain_golden.lua\",\"pc\":1,\"op\":\"ADD\",\"a\":1,\"b\":0,\"c\":257,\"bx\":257,\"sbx\":-130814,\"line\":2,\"source\":\"trace_plain_golden.lua\",\"callDepth\":1,\"registers\":[{\"slot\":0,\"name\":\"x\",\"value\":1,\"type\":\"number\"},{\"slot\":1,\"name\":null,\"value\":null,\"type\":\"nil\"}]}\n"
        "{\"seq\":2,\"kind\":\"instruction\",\"funcName\":\"trace_plain_golden.lua\",\"pc\":2,\"op\":\"MOVE\",\"a\":0,\"b\":1,\"c\":0,\"bx\":512,\"sbx\":-130559,\"line\":2,\"source\":\"trace_plain_golden.lua\",\"callDepth\":1,\"registers\":[{\"slot\":0,\"name\":\"x\",\"value\":1,\"type\":\"number\"},{\"slot\":1,\"name\":null,\"value\":3,\"type\":\"number\"}]}\n"
        "{\"seq\":3,\"kind\":\"instruction\",\"funcName\":\"trace_plain_golden.lua\",\"pc\":3,\"op\":\"MOVE\",\"a\":1,\"b\":0,\"c\":0,\"bx\":0,\"sbx\":-131071,\"line\":3,\"source\":\"trace_plain_golden.lua\",\"callDepth\":1,\"registers\":[{\"slot\":0,\"name\":\"x\",\"value\":3,\"type\":\"number\"},{\"slot\":1,\"name\":null,\"value\":3,\"type\":\"number\"}]}\n"
        "{\"seq\":4,\"kind\":\"instruction\",\"funcName\":\"trace_plain_golden.lua\",\"pc\":4,\"op\":\"RETURN\",\"a\":1,\"b\":2,\"c\":0,\"bx\":1024,\"sbx\":-130047,\"line\":3,\"source\":\"trace_plain_golden.lua\",\"callDepth\":1,\"registers\":[{\"slot\":0,\"name\":\"x\",\"value\":3,\"type\":\"number\"},{\"slot\":1,\"name\":null,\"value\":3,\"type\":\"number\"}]}\n"
        "{\"seq\":5,\"kind\":\"return\",\"funcName\":\"trace_plain_golden.lua\",\"source\":\"trace_plain_golden.lua\",\"line\":3,\"callDepth\":1}\n";

    ASSERT_EQ(suite, expected, actual, "plain trace JSONL matches golden output");
    std::filesystem::remove(path);
}

void testTraceJsonlDiffGolden(TestSuite& suite) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "lua_cpp_trace_diff_golden_test.jsonl";

    const Str actual = writeTraceGoldenJsonl(path, "trace_diff_golden.lua", true);
    const Str expected =
        "{\"seq\":0,\"kind\":\"instruction\",\"funcName\":\"trace_diff_golden.lua\",\"pc\":0,\"op\":\"LOADK\",\"a\":0,\"b\":0,\"c\":0,\"bx\":0,\"sbx\":-131071,\"line\":1,\"source\":\"trace_diff_golden.lua\",\"callDepth\":1,\"changedRegisters\":[{\"slot\":0,\"name\":null,\"old\":null,\"new\":1,\"oldType\":\"nil\",\"newType\":\"number\"}]}\n"
        "{\"seq\":1,\"kind\":\"instruction\",\"funcName\":\"trace_diff_golden.lua\",\"pc\":1,\"op\":\"ADD\",\"a\":1,\"b\":0,\"c\":257,\"bx\":257,\"sbx\":-130814,\"line\":2,\"source\":\"trace_diff_golden.lua\",\"callDepth\":1,\"changedRegisters\":[{\"slot\":1,\"name\":null,\"old\":null,\"new\":3,\"oldType\":\"nil\",\"newType\":\"number\"}]}\n"
        "{\"seq\":2,\"kind\":\"instruction\",\"funcName\":\"trace_diff_golden.lua\",\"pc\":2,\"op\":\"MOVE\",\"a\":0,\"b\":1,\"c\":0,\"bx\":512,\"sbx\":-130559,\"line\":2,\"source\":\"trace_diff_golden.lua\",\"callDepth\":1,\"changedRegisters\":[{\"slot\":0,\"name\":\"x\",\"old\":1,\"new\":3,\"oldType\":\"number\",\"newType\":\"number\"}]}\n"
        "{\"seq\":3,\"kind\":\"instruction\",\"funcName\":\"trace_diff_golden.lua\",\"pc\":3,\"op\":\"MOVE\",\"a\":1,\"b\":0,\"c\":0,\"bx\":0,\"sbx\":-131071,\"line\":3,\"source\":\"trace_diff_golden.lua\",\"callDepth\":1,\"changedRegisters\":[]}\n"
        "{\"seq\":4,\"kind\":\"return\",\"funcName\":\"trace_diff_golden.lua\",\"source\":\"trace_diff_golden.lua\",\"line\":3,\"callDepth\":1}\n"
        "{\"seq\":5,\"kind\":\"instruction\",\"funcName\":\"trace_diff_golden.lua\",\"pc\":4,\"op\":\"RETURN\",\"a\":1,\"b\":2,\"c\":0,\"bx\":1024,\"sbx\":-130047,\"line\":3,\"source\":\"trace_diff_golden.lua\",\"callDepth\":1,\"changedRegisters\":[{\"slot\":0,\"name\":\"x\",\"old\":3,\"new\":null,\"oldType\":\"number\",\"newType\":\"nil\"},{\"slot\":1,\"name\":null,\"old\":3,\"new\":null,\"oldType\":\"number\",\"newType\":\"nil\"}]}\n";

    ASSERT_EQ(suite, expected, actual, "trace-diff JSONL matches golden output");
    std::filesystem::remove(path);
}

void registerVMTraceDebugTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Trace Events Keep Instruction Call Return Order",
                          testTraceEventsKeepInstructionCallReturnOrder);
    registry.registerTest(kSuiteName, "Debug Hooks Keep Count Line Call Return Order",
                          testDebugHooksKeepCountLineCallReturnOrder);
    registry.registerTest(kSuiteName, "JsonTraceSink Writes Stable Json Lines",
                          testJsonTraceSinkWritesStableJsonLines);
    registry.registerTest(kSuiteName, "Trace Diff Writes Changed Registers",
                          testTraceDiffWritesChangedRegisters);
    registry.registerTest(kSuiteName, "Trace JSONL Plain Golden",
                          testTraceJsonlPlainGolden);
    registry.registerTest(kSuiteName, "Trace JSONL Diff Golden",
                          testTraceJsonlDiffGolden);
}
