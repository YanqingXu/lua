/**
 * @file test_breakpoints.cpp
 * @brief Breakpoint replacement, binding, and VM-hit integration tests.
 */

#include "../framework/test_framework.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "debugger/breakpoint_manager.hpp"
#include "debugger/debug_runtime.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

using namespace Lua;
using namespace Lua::Debugger;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Debugger Breakpoints";

Proto* compileBreakpointChunk(RuntimeServices& services, StrView source, StrView sourceName) {
    Parser parser{Str(source), services};
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    CodeGenerator codegen(services);
    return codegen.generate(*parsed, sourceName);
}

Function* createBreakpointFunction(RuntimeServices& services, LuaState* state, Proto* proto) {
    Function* function = new Function(proto);
    function->setEnv(state->getGlobalTable());
    services.gc.registerObject(function);
    return function;
}

class BreakpointEventSink final : public IDebugEventSink {
public:
    void onDebugStateChanged(const DebugSessionSnapshot&) override {}

    void onBreakpointChanged(const BreakpointBinding& breakpoint) override {
        std::lock_guard lock(mutex_);
        changed_.push_back(breakpoint);
    }

    void onDebugOutput(StrView output, DebugOutputCategory) override {
        std::lock_guard lock(mutex_);
        output_.emplace_back(output);
    }

    [[nodiscard]] bool sawVerifiedLine(i32 line) const {
        std::lock_guard lock(mutex_);
        for (const BreakpointBinding& breakpoint : changed_) {
            if (breakpoint.verified && breakpoint.line == line) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] usize outputCount() const {
        std::lock_guard lock(mutex_);
        return output_.size();
    }

private:
    mutable std::mutex mutex_;
    Vec<BreakpointBinding> changed_;
    Vec<Str> output_;
};

} // namespace

void testBreakpointPendingBindingAndReplacement(TestSuite& suite) {
    constexpr StrView source = "-- comment\n"
                               "\n"
                               "local total = seed + 1\n"
                               "local function add(value)\n"
                               "    total = total + value\n"
                               "end\n"
                               "for i = 1, 2 do\n"
                               "    add(i)\n"
                               "end\n"
                               "return total\n";
    constexpr StrView sourceName = "@C:\\Game\\scripts\\breakpoints.lua";
    RuntimeServices services = RuntimeServices::fromSingletons();
    Proto* proto = compileBreakpointChunk(services, source, sourceName);
    BreakpointManager manager;
    const SourceId sourceId = manager.registerFilePath("c:/game/scripts/breakpoints.lua");

    const std::array initialRequests{SourceBreakpoint{1}, SourceBreakpoint{999}};
    auto initial = manager.setBreakpoints(sourceId, initialRequests);
    ASSERT_TRUE(suite, initial && initial->size() == 2, "setBreakpoints accepts a replace-all source list");
    ASSERT_TRUE(suite, initial && !initial->at(0).verified,
                "Breakpoint created before Proto load remains pending and unverified");
    ASSERT_TRUE(suite, initial && initial->at(0).message.starts_with("pending:"),
                "Pending breakpoint has a stable diagnostic");

    const Vec<BreakpointBinding> changed = manager.registerProto(*proto);
    ASSERT_TRUE(suite, !changed.empty(), "Loading the Proto tree reports pending binding changes");
    ASSERT_TRUE(suite, changed.front().verified && changed.front().line == 3,
                "Comment/blank request moves to the next executable line");

    DebugInfoIndex index(*proto);
    usize lineThreeLocations = 0;
    bool everyLineThreePcMatches = true;
    for (const DebugCodeLocation& location : index.allLocations()) {
        if (location.line == 3) {
            ++lineThreeLocations;
            everyLineThreePcMatches =
                everyLineThreePcMatches && manager.match(*location.proto, location.pc) != nullptr;
        }
    }
    ASSERT_TRUE(suite, lineThreeLocations > 1, "Fixture exposes multiple bytecode PCs on one source line");
    ASSERT_TRUE(suite, everyLineThreePcMatches, "Binding covers every PC for the verified source line");

    const std::array replacements{SourceBreakpoint{8}};
    auto replaced = manager.setBreakpoints(sourceId, replacements);
    ASSERT_TRUE(suite, replaced && replaced->size() == 1 && replaced->front().verified,
                "Replacing a loaded source returns its current verified binding");
    bool oldLineRemoved = true;
    bool replacementPresent = false;
    for (const DebugCodeLocation& location : index.allLocations()) {
        if (location.line == 3) {
            oldLineRemoved = oldLineRemoved && !manager.match(*location.proto, location.pc);
        }
        if (location.line == 8) {
            replacementPresent = replacementPresent || manager.match(*location.proto, location.pc) != nullptr;
        }
    }
    ASSERT_TRUE(suite, oldLineRemoved, "Replace-all removes every old instruction binding immediately");
    ASSERT_TRUE(suite, replacementPresent, "Replacement breakpoint is installed immediately");

    const std::array<SourceBreakpoint, 0> noBreakpoints{};
    auto cleared = manager.setBreakpoints(sourceId, noBreakpoints);
    ASSERT_TRUE(suite, cleared && cleared->empty(), "Empty replace-all list clears the source breakpoints");
    bool allCleared = true;
    for (const DebugCodeLocation& location : index.allLocations()) {
        allCleared = allCleared && !manager.match(*location.proto, location.pc);
    }
    ASSERT_TRUE(suite, allCleared, "Cleared source leaves no stale Proto/PC entries");
}

void testBreakpointNestedProtoAndInvalidSource(TestSuite& suite) {
    constexpr StrView source = "local function outer()\n"
                               "    local function inner()\n"
                               "        return 9\n"
                               "    end\n"
                               "    return inner()\n"
                               "end\n"
                               "return outer()\n";
    RuntimeServices services = RuntimeServices::fromSingletons();
    Proto* proto = compileBreakpointChunk(services, source, "@nested_breakpoint.lua");
    BreakpointManager manager;
    [[maybe_unused]] const Vec<BreakpointBinding> initialChanges = manager.registerProto(*proto);
    const SourceId sourceId = manager.registerFilePath("nested_breakpoint.lua");
    const std::array requests{SourceBreakpoint{3}};
    auto bound = manager.setBreakpoints(sourceId, requests);

    ASSERT_TRUE(suite, bound && bound->front().verified && bound->front().line == 3,
                "Breakpoint resolves inside a nested Proto using inherited source identity");
    DebugInfoIndex index(*proto);
    bool nestedMatched = false;
    for (const DebugCodeLocation& location : index.allLocations()) {
        if (location.line == 3 && location.proto != proto) {
            nestedMatched = nestedMatched || manager.match(*location.proto, location.pc) != nullptr;
        }
    }
    ASSERT_TRUE(suite, nestedMatched, "Nested Proto instructions participate in breakpoint lookup");
    ASSERT_TRUE(suite, !manager.setBreakpoints(SourceId{9999}, requests),
                "Unknown SourceId is rejected deterministically");
}

void testAdvancedBreakpointBindingAndHitSemantics(TestSuite& suite) {
    constexpr StrView source = "local function worker(value)\n"
                               "    return value + 1\n"
                               "end\n"
                               "return worker(4)\n";
    RuntimeServices services = RuntimeServices::fromSingletons();
    Proto* first = compileBreakpointChunk(services, source, "@advanced_breakpoints.lua");
    Proto* active = first;
    BreakpointManager manager;
    const SourceId sourceId = manager.registerFilePath("advanced_breakpoints.lua");
    [[maybe_unused]] const Vec<BreakpointBinding> firstChanges = manager.registerProto(*first);

    const std::array counted{SourceBreakpoint{2, {}, "2", {}}};
    auto countedBindings = manager.setBreakpoints(sourceId, counted);
    ASSERT_TRUE(suite, countedBindings && countedBindings->front().verified,
                "A positive decimal hit condition is accepted");

    DebugInfoIndex firstIndex(*first);
    Ptr<const BreakpointHitList> firstHits;
    for (const DebugCodeLocation& location : firstIndex.allLocations()) {
        if (location.line == 2) {
            firstHits = manager.match(*location.proto, location.pc);
            if (firstHits != nullptr && !firstHits->empty()) {
                break;
            }
        }
    }
    ASSERT_TRUE(suite, firstHits != nullptr && firstHits->size() == 1,
                "The counted breakpoint has an immutable instruction hit entry");
    if (firstHits != nullptr && !firstHits->empty()) {
        const BreakpointId id = firstHits->front().id;
        auto firstHit = manager.recordHit(id);
        auto secondHit = manager.recordHit(id);
        ASSERT_TRUE(suite, firstHit && firstHit->hitCount == 1 && !firstHit->hitTargetReached,
                    "Hit count is global per breakpoint and does not activate before exact N");
        ASSERT_TRUE(suite, secondHit && secondHit->hitCount == 2 && secondHit->hitTargetReached,
                    "Hit count activates on the exact requested logical hit");

        Proto* reloaded = compileBreakpointChunk(services, source, "@advanced_breakpoints.lua");
        active = reloaded;
        [[maybe_unused]] const Vec<BreakpointBinding> reloadChanges = manager.registerProto(*reloaded);
        auto thirdHit = manager.recordHit(id);
        ASSERT_TRUE(suite, thirdHit && thirdHit->hitCount == 3 && !thirdHit->hitTargetReached,
                    "Registering another Proto for the same source preserves the logical hit counter");
    }

    const std::array invalid{SourceBreakpoint{2, {}, "twice", {}}};
    auto invalidBindings = manager.setBreakpoints(sourceId, invalid);
    ASSERT_TRUE(suite, invalidBindings && !invalidBindings->front().verified &&
                           invalidBindings->front().message.find("positive decimal") != Str::npos,
                "Invalid hit conditions are returned as unverified bindings with a stable reason");

    const std::array functions{FunctionBreakpoint{"worker", {}, {}}};
    auto functionBindings = manager.setFunctionBreakpoints(functions);
    ASSERT_TRUE(suite, functionBindings && functionBindings->front().verified &&
                           functionBindings->front().functionName == Opt<Str>{"worker"},
                "Function breakpoints bind through the compiler-emitted function name");

    const std::array sourceAtEntry{SourceBreakpoint{2}};
    auto sourceBindings = manager.setBreakpoints(sourceId, sourceAtEntry);
    ASSERT_TRUE(suite, sourceBindings && sourceBindings->front().verified,
                "A source breakpoint can coexist with a function breakpoint");
    bool combinedHit = false;
    DebugInfoIndex activeIndex(*active);
    for (const DebugCodeLocation& location : activeIndex.allLocations()) {
        Ptr<const BreakpointHitList> hits = manager.match(*location.proto, location.pc);
        combinedHit = combinedHit || (location.line == 2 && hits != nullptr && hits->size() >= 2);
    }
    ASSERT_TRUE(suite, combinedHit,
                "Source and function breakpoints at one instruction are retained instead of overwriting each other");
}

void testBreakpointLoopHitsWithoutConsecutiveDuplicates(TestSuite& suite) {
    constexpr StrView source = "local total = 0\n"
                               "for i = 1, 2 do\n"
                               "    total = total + i\n"
                               "end\n"
                               "return total\n";
    EngineContext context;
    DebugController& controller = context.globalState().enableDebugger();
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    Proto* proto = compileBreakpointChunk(services, source, "@debugger/loop_breakpoint.lua");
    Function* function = createBreakpointFunction(services, state.get(), proto);

    const SourceId sourceId = controller.registerFilePath("debugger/loop_breakpoint.lua");
    const std::array requests{SourceBreakpoint{3}};
    auto pending = controller.setBreakpoints(sourceId, requests);
    Ptr<BreakpointEventSink> sink = makePtr<BreakpointEventSink>();
    auto attached = controller.attachSession(sink);
    DebugSession session = std::move(*attached);
    const bool configured = controller.configurationDone().has_value();

    std::atomic<bool> executionDone = false;
    usize hitCount = 0;
    bool timedOut = false;
    std::thread control([&]() {
        PauseGeneration handledGeneration;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!executionDone.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
            const DebugSessionSnapshot snapshot = controller.snapshot();
            if (snapshot.state == DebugSessionState::Suspended && snapshot.stopReason == DebugStopReason::Breakpoint &&
                snapshot.pauseGeneration != handledGeneration) {
                handledGeneration = snapshot.pauseGeneration;
                ++hitCount;
                (void)controller.continueExecution(DebugController::mainThreadId());
            } else {
                std::this_thread::yield();
            }
        }
        if (!executionDone.load(std::memory_order_acquire)) {
            timedOut = true;
            (void)controller.terminateExecution();
        }
    });

    bool executed = true;
    try {
        VM::execute(services, state.get(), function);
    } catch (const RuntimeError&) {
        executed = false;
    }
    executionDone.store(true, std::memory_order_release);
    control.join();

    ASSERT_TRUE(suite, configured && pending && !pending->front().verified,
                "Loop breakpoint is accepted as pending before the first VM entry");
    ASSERT_FALSE(suite, timedOut, "Breakpoint loop execution converges within its timeout");
    ASSERT_TRUE(suite, executed && state->top().isNumber() && state->top().asNumber() == 3,
                "Program completes correctly after repeated breakpoint resumes");
    ASSERT_EQ(suite, usize{2}, hitCount, "Loop revisits stop twice while consecutive same-line PCs are suppressed");
    ASSERT_TRUE(suite, sink->sawVerifiedLine(3), "Pending breakpoint emits a verified changed notification on load");

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    state.reset();
    context.gc().clearAll(context.strings());
}

void testHotReloadRetiresAndRebindsProtoLocations(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    BreakpointManager manager;
    const SourceId sourceId = manager.registerFilePath("hot_reload.lua");
    const std::array requested{SourceBreakpoint{2}};
    auto pending = manager.setBreakpoints(sourceId, requested);

    Proto* first = compileBreakpointChunk(services, "local value = 1\nreturn value\n", "@hot_reload.lua");
    auto firstChanges = manager.registerProto(*first);
    const Opt<u64> firstIdentity = manager.sourceContentIdentity(sourceId);
    DebugInfoIndex firstIndex(*first);
    bool firstBound = false;
    for (const DebugCodeLocation& location : firstIndex.allLocations()) {
        firstBound = firstBound || manager.match(*location.proto, location.pc) != nullptr;
    }

    Proto* second = compileBreakpointChunk(services,
                                           "local value = 2\n-- inserted by reload\nreturn value\n",
                                           "@hot_reload.lua");
    auto secondChanges = manager.registerProto(*second);
    const Opt<u64> secondIdentity = manager.sourceContentIdentity(sourceId);
    bool oldRetired = true;
    for (const DebugCodeLocation& location : firstIndex.allLocations()) {
        oldRetired = oldRetired && manager.match(*location.proto, location.pc) == nullptr;
    }
    DebugInfoIndex secondIndex(*second);
    bool secondBoundAtMovedLine = false;
    for (const DebugCodeLocation& location : secondIndex.allLocations()) {
        if (Ptr<const BreakpointHitList> hits = manager.match(*location.proto, location.pc)) {
            secondBoundAtMovedLine = secondBoundAtMovedLine || location.line == 3;
        }
    }

    Proto* stripped = compileBreakpointChunk(services, "-- no executable source line requested\n", "@hot_reload.lua");
    stripped->getLineInfo().clear();
    auto strippedChanges = manager.registerProto(*stripped);
    bool newRetired = true;
    for (const DebugCodeLocation& location : secondIndex.allLocations()) {
        newRetired = newRetired && manager.match(*location.proto, location.pc) == nullptr;
    }

    ASSERT_TRUE(suite, pending && !pending->front().verified && !firstChanges.empty() && firstBound,
                "The initial source version resolves a pending breakpoint");
    ASSERT_TRUE(suite, firstIdentity && secondIdentity && *firstIdentity != *secondIdentity,
                "Bytecode, line metadata, and constants produce a stable changed source content identity");
    ASSERT_TRUE(suite, oldRetired && !secondChanges.empty() && secondChanges.front().verified &&
                           secondChanges.front().line == 3 && secondBoundAtMovedLine,
                "Reload retires old Proto PCs, rebinds the new source line, and reports breakpoint changed");
    ASSERT_TRUE(suite, newRetired && !strippedChanges.empty() && !strippedChanges.front().verified &&
                           strippedChanges.front().message.find("no line information") != Str::npos,
                "A stripped reload removes stale PCs and reports why the breakpoint cannot be rebound");
}

void testLogPointInterpolationAndRateLimit(TestSuite& suite) {
    constexpr StrView source = "local total = 0\n"
                               "for index = 1, 105 do\n"
                               "    total = total + index\n"
                               "end\n"
                               "return total\n";
    EngineContext context;
    DebugController& controller = context.globalState().enableDebugger();
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    Proto* proto = compileBreakpointChunk(services, source, "@debugger/log_point.lua");
    Function* function = createBreakpointFunction(services, state.get(), proto);

    const SourceId sourceId = controller.registerFilePath("debugger/log_point.lua");
    SourceBreakpoint logPoint;
    logPoint.line = 3;
    logPoint.logMessage = "index={index} total={total}";
    const std::array requests{logPoint};
    auto pending = controller.setBreakpoints(sourceId, requests);
    Ptr<BreakpointEventSink> sink = makePtr<BreakpointEventSink>();
    auto attached = controller.attachSession(sink);
    DebugSession session = std::move(*attached);
    const bool configured = controller.configurationDone().has_value();

    bool executed = true;
    try {
        VM::execute(services, state.get(), function);
    } catch (const RuntimeError&) {
        executed = false;
    }
    const DebugSessionSnapshot diagnostics = controller.snapshot();

    ASSERT_TRUE(suite, configured && pending && !pending->front().verified,
                "A log point can be installed before its source is loaded");
    ASSERT_TRUE(suite, executed && state->top().isNumber() && state->top().asNumber() == 5565,
                "Log interpolation remains read-only and does not alter program execution");
    ASSERT_EQ(suite, usize{100}, sink->outputCount(),
              "Log point output is capped by the configured per-second flood limit");
    ASSERT_EQ(suite, u64{5}, diagnostics.droppedLogMessages,
              "Log point throttling exposes a cumulative metadata-only dropped-message count");

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    state.reset();
    context.gc().clearAll(context.strings());
}

void registerDebuggerBreakpointTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Pending Binding And Replacement", testBreakpointPendingBindingAndReplacement);
    registry.registerTest(kSuiteName, "Nested Proto And Invalid Source", testBreakpointNestedProtoAndInvalidSource);
    registry.registerTest(kSuiteName, "Advanced Binding And Hit Semantics",
                          testAdvancedBreakpointBindingAndHitSemantics);
    registry.registerTest(kSuiteName, "Loop Hits Without Consecutive Duplicates",
                          testBreakpointLoopHitsWithoutConsecutiveDuplicates);
    registry.registerTest(kSuiteName, "Log Point Interpolation And Rate Limit",
                          testLogPointInterpolationAndRateLimit);
    registry.registerTest(kSuiteName, "Hot Reload Retires And Rebinds Proto Locations",
                          testHotReloadRetiresAndRebindsProtoLocations);
}
