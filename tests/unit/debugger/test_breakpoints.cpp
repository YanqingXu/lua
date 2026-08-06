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

    [[nodiscard]] bool sawVerifiedLine(i32 line) const {
        std::lock_guard lock(mutex_);
        for (const BreakpointBinding& breakpoint : changed_) {
            if (breakpoint.verified && breakpoint.line == line) {
                return true;
            }
        }
        return false;
    }

private:
    mutable std::mutex mutex_;
    Vec<BreakpointBinding> changed_;
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
                everyLineThreePcMatches && manager.match(*location.proto, location.pc).has_value();
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
            replacementPresent = replacementPresent || manager.match(*location.proto, location.pc).has_value();
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
            nestedMatched = nestedMatched || manager.match(*location.proto, location.pc).has_value();
        }
    }
    ASSERT_TRUE(suite, nestedMatched, "Nested Proto instructions participate in breakpoint lookup");
    ASSERT_TRUE(suite, !manager.setBreakpoints(SourceId{9999}, requests),
                "Unknown SourceId is rejected deterministically");
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

void registerDebuggerBreakpointTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Pending Binding And Replacement", testBreakpointPendingBindingAndReplacement);
    registry.registerTest(kSuiteName, "Nested Proto And Invalid Source", testBreakpointNestedProtoAndInvalidSource);
    registry.registerTest(kSuiteName, "Loop Hits Without Consecutive Duplicates",
                          testBreakpointLoopHitsWithoutConsecutiveDuplicates);
}
