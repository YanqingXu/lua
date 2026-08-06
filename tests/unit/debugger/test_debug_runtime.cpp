/**
 * @file test_debug_runtime.cpp
 * @brief Debug runtime attachment and lifecycle contract tests.
 */

#include "../framework/test_framework.hpp"

#include "debugger/debug_runtime.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

using namespace Lua;
using namespace Lua::Debugger;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Debugger Runtime Lifecycle";

class RecordingDebugSink final : public IDebugEventSink {
public:
    void onDebugStateChanged(const DebugSessionSnapshot& snapshot) override {
        std::lock_guard lock(mutex);
        events.push_back(snapshot);
    }

    void onDebugSemanticEvent(DebugSemanticEvent event) override {
        std::lock_guard lock(mutex);
        semanticEvents.push_back(event);
    }

    void onDebugExecutionUnitChanged(const DebugState& state, bool started) override {
        std::lock_guard lock(mutex);
        executionUnits.emplace_back(state, started);
    }

    [[nodiscard]] usize eventCount() const {
        std::lock_guard lock(mutex);
        return events.size();
    }

    [[nodiscard]] bool sawSemantic(DebugSemanticEvent event) const {
        std::lock_guard lock(mutex);
        return std::find(semanticEvents.begin(), semanticEvents.end(), event) != semanticEvents.end();
    }

    mutable std::mutex mutex;
    Vec<DebugSessionSnapshot> events;
    Vec<DebugSemanticEvent> semanticEvents;
    Vec<std::pair<DebugState, bool>> executionUnits;
};

void noOpApiHook(::lua_State*, ::lua_Debug*) {}

std::atomic<bool> gNativeCallEntered = false;
std::atomic<bool> gReleaseNativeCall = false;

i32 blockingNativeCall(LuaState*) {
    gNativeCallEntered.store(true, std::memory_order_release);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!gReleaseNativeCall.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    return 0;
}

Proto* compileDebugChunk(RuntimeServices& services, StrView source, StrView sourceName) {
    Parser parser{Str(source), services};
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    CodeGenerator codegen(services);
    return codegen.generate(*parsed, sourceName);
}

Function* createDebugFunction(RuntimeServices& services, LuaState* state, Proto* proto) {
    Function* function = new Function(proto);
    function->setEnv(state->getGlobalTable());
    services.gc.registerObject(function);
    return function;
}

bool waitForState(DebugController& controller, DebugSessionState expected,
                  std::chrono::milliseconds timeout = std::chrono::seconds(3)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (controller.snapshot().state == expected) {
            return true;
        }
        std::this_thread::yield();
    }
    return controller.snapshot().state == expected;
}

bool waitForFlag(const std::atomic<bool>& flag, std::chrono::milliseconds timeout = std::chrono::seconds(3)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (flag.load(std::memory_order_acquire)) {
            return true;
        }
        std::this_thread::yield();
    }
    return flag.load(std::memory_order_acquire);
}

} // namespace

void testDebuggerRuntimeStateMachine(TestSuite& suite) {
    DebugController controller;
    Ptr<RecordingDebugSink> sink = makePtr<RecordingDebugSink>();
    auto attached = controller.attachSession(sink);
    ASSERT_TRUE(suite, attached.has_value(), "A debugger client can attach to an idle controller");
    ASSERT_TRUE(suite, attached && attached->attached(), "Session token reports its active attachment");
    ASSERT_TRUE(suite, controller.snapshot().state == DebugSessionState::Starting,
                "Attached session starts before configurationDone");

    ASSERT_TRUE(suite, controller.configurationDone().has_value(), "configurationDone starts execution");
    ASSERT_TRUE(suite, controller.pause(DebugController::mainThreadId()).has_value(),
                "Running session accepts a pause request");
    ASSERT_TRUE(suite, controller.pauseRequested(), "Pause request is visible through the atomic safepoint probe");
    ASSERT_TRUE(suite, controller.snapshot().state == DebugSessionState::PauseRequested,
                "Pause remains pending until the VM reaches a safepoint");

    ASSERT_TRUE(suite, controller.notifySuspended(DebugStopReason::Pause).has_value(),
                "VM safepoint confirms the pending pause");
    const DebugSessionSnapshot suspended = controller.snapshot();
    ASSERT_TRUE(suite, suspended.state == DebugSessionState::Suspended, "Safepoint publishes suspended state");
    ASSERT_TRUE(suite, suspended.stopReason == DebugStopReason::Pause, "Suspended snapshot preserves stop reason");
    ASSERT_TRUE(suite, suspended.pauseGeneration.valid(), "Each suspension owns a non-zero pause generation");

    ASSERT_TRUE(suite, controller.continueExecution(DebugController::mainThreadId()).has_value(),
                "Suspended session accepts continue");
    ASSERT_TRUE(suite, controller.snapshot().state == DebugSessionState::ResumeRequested,
                "Continue waits for VM resume confirmation");
    ASSERT_TRUE(suite, controller.confirmResumed().has_value(), "VM confirms resume exactly once");
    ASSERT_TRUE(suite, controller.snapshot().state == DebugSessionState::Running,
                "Resume confirmation returns session to running");

    controller.notifyProgramTerminated(DebugTerminationReason::Completed);
    ASSERT_TRUE(suite, controller.snapshot().state == DebugSessionState::Terminated,
                "Normal program completion reaches the terminal state");
    ASSERT_TRUE(suite, controller.snapshot().terminationReason == DebugTerminationReason::Completed,
                "Normal completion reason is retained");
    ASSERT_TRUE(suite, sink->eventCount() >= 7, "Lifecycle changes are published as copied snapshots");
}

void testDebuggerSafepointExecutesCurrentInstructionOnce(TestSuite& suite) {
    DebugController controller;
    auto attached = controller.attachSession();
    DebugSession session = std::move(*attached);
    ASSERT_TRUE(suite, controller.configurationDone().has_value(), "Safepoint fixture starts running");
    ASSERT_TRUE(suite, controller.pause(DebugController::mainThreadId()).has_value(),
                "Safepoint fixture queues pause before the instruction");

    std::atomic<i32> executed = 0;
    DebugSafepointResult result = DebugSafepointResult::TerminateExecution;
    std::thread vmOwner([&]() {
        result = controller.instructionSafepoint();
        if (result == DebugSafepointResult::ContinueExecution) {
            executed.fetch_add(1, std::memory_order_release);
        }
    });

    const bool suspended = waitForState(controller, DebugSessionState::Suspended);
    ASSERT_TRUE(suite, suspended, "VM reaches suspended state within the timeout");
    ASSERT_EQ(suite, i32{0}, executed.load(std::memory_order_acquire),
              "The current instruction has not executed while stopped");
    ASSERT_TRUE(suite, controller.continueExecution(DebugController::mainThreadId()).has_value(),
                "Control thread wakes the suspended VM");
    vmOwner.join();

    ASSERT_TRUE(suite, result == DebugSafepointResult::ContinueExecution,
                "Continue releases the owner-thread safepoint normally");
    ASSERT_EQ(suite, i32{1}, executed.load(std::memory_order_acquire),
              "The instruction following the safepoint executes exactly once");
    ASSERT_TRUE(suite, controller.snapshot().state == DebugSessionState::Running,
                "Owner thread confirms running state when it leaves the pause loop");
}

void testDebuggerSafepointStress(TestSuite& suite) {
    DebugController controller;
    auto attached = controller.attachSession();
    DebugSession session = std::move(*attached);
    const bool configured = controller.configurationDone().has_value();
    std::atomic<bool> workerExited = false;
    std::atomic<usize> safepoints = 0;

    std::thread vmOwner([&]() {
        for (;;) {
            if (controller.instructionSafepoint() == DebugSafepointResult::TerminateExecution) {
                break;
            }
            safepoints.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::yield();
        }
        workerExited.store(true, std::memory_order_release);
    });

    bool converged = configured;
    constexpr usize kIterations = 1000;
    for (usize iteration = 0; iteration < kIterations && converged; ++iteration) {
        converged = controller.pause(DebugController::mainThreadId()).has_value() &&
                    waitForState(controller, DebugSessionState::Suspended) &&
                    controller.continueExecution(DebugController::mainThreadId()).has_value() &&
                    waitForState(controller, DebugSessionState::Running);
    }

    const bool terminated = controller.terminateExecution().has_value();
    vmOwner.join();
    ASSERT_TRUE(suite, converged, "1,000 pause/continue cycles converge without a lost wakeup");
    ASSERT_TRUE(suite, terminated, "Stress fixture terminates an active running session");
    ASSERT_TRUE(suite, workerExited.load(std::memory_order_acquire), "Terminate wakes and joins the VM owner thread");
    ASSERT_TRUE(suite, safepoints.load(std::memory_order_relaxed) > 0,
                "Stress worker makes forward progress between pauses");
}

void testDebuggerSafepointTerminationRaces(TestSuite& suite) {
    auto runRace = [&](int variant) {
        DebugController controller;
        auto attached = controller.attachSession();
        DebugSession session = std::move(*attached);
        if (!controller.configurationDone() || !controller.pause(DebugController::mainThreadId())) {
            return false;
        }

        DebugSafepointResult result = DebugSafepointResult::ContinueExecution;
        std::thread vmOwner([&]() { result = controller.instructionSafepoint(); });
        if (!waitForState(controller, DebugSessionState::Suspended)) {
            controller.shutdown(DisconnectAction::TerminateExecution);
            vmOwner.join();
            return false;
        }

        if (variant == 0) {
            (void)controller.terminateExecution();
        } else if (variant == 1) {
            session.disconnect(DisconnectAction::ContinueExecution);
        } else {
            controller.notifyProgramTerminated(DebugTerminationReason::Completed);
        }
        vmOwner.join();

        if (variant == 1) {
            return result == DebugSafepointResult::ContinueExecution;
        }
        return result == DebugSafepointResult::TerminateExecution;
    };

    ASSERT_TRUE(suite, runRace(0), "Terminate racing a paused VM wakes it with terminate result");
    ASSERT_TRUE(suite, runRace(1), "Continue-on-disconnect wakes a paused VM without terminating it");
    ASSERT_TRUE(suite, runRace(2), "Program exit racing a pause wakes the VM and reaches terminal state");
}

void testDebuggerVmSafepointAndSemanticIntegration(TestSuite& suite) {
    EngineContext context;
    DebugController& controller = context.globalState().enableDebugger();
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    Proto* proto = compileDebugChunk(services, "local function twice(x) return x * 2 end\nreturn twice(21)\n",
                                     "@debugger/safepoint.lua");
    Function* function = createDebugFunction(services, state.get(), proto);

    Ptr<RecordingDebugSink> sink = makePtr<RecordingDebugSink>();
    auto attached = controller.attachSession(sink);
    DebugSession session = std::move(*attached);
    const bool configured = controller.configurationDone().has_value();
    const bool pauseQueued = controller.pause(DebugController::mainThreadId()).has_value();
    bool controlCompleted = false;
    std::thread control([&]() {
        if (waitForState(controller, DebugSessionState::Suspended)) {
            controlCompleted = controller.continueExecution(DebugController::mainThreadId()).has_value();
        }
    });

    VM::execute(services, state.get(), function);
    control.join();

    ASSERT_TRUE(suite, configured && pauseQueued && controlCompleted,
                "Real VM execution pauses at an instruction boundary and resumes within timeout");
    ASSERT_TRUE(suite, state->top().isNumber() && state->top().asNumber() == 42,
                "Resumed bytecode completes with the expected result");
    ASSERT_TRUE(suite, sink->sawSemantic(DebugSemanticEvent::FunctionEnter),
                "Debugger observes function-enter after frame creation");
    ASSERT_TRUE(suite, sink->sawSemantic(DebugSemanticEvent::FunctionReturn),
                "Debugger observes function-return before frame destruction");

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    state.reset();
    context.gc().clearAll(context.strings());
}

void testDebuggerSemanticEventsAndExceptionBoundary(TestSuite& suite) {
    EngineContext context;
    DebugController& controller = context.globalState().enableDebugger();
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    Proto* proto = compileDebugChunk(services, "local value = nil\nreturn value + 1\n", "=debug_exception");
    Function* function = createDebugFunction(services, state.get(), proto);
    Ptr<RecordingDebugSink> sink = makePtr<RecordingDebugSink>();
    auto attached = controller.attachSession(sink);
    DebugSession session = std::move(*attached);
    (void)controller.configurationDone();

    bool failed = false;
    try {
        VM::execute(services, state.get(), function);
    } catch (const RuntimeError&) {
        failed = true;
    }
    (void)controller.semanticSafepoint(DebugSemanticEvent::CoroutineResume);
    (void)controller.semanticSafepoint(DebugSemanticEvent::CoroutineYield);

    ASSERT_TRUE(suite, failed, "Exception-boundary fixture raises a runtime error");
    ASSERT_TRUE(suite, sink->sawSemantic(DebugSemanticEvent::Exception),
                "Runtime error is reported before executeProto unwinds");
    ASSERT_TRUE(suite, sink->sawSemantic(DebugSemanticEvent::CoroutineResume),
                "Coroutine resume boundary has a protocol-independent semantic event");
    ASSERT_TRUE(suite, sink->sawSemantic(DebugSemanticEvent::CoroutineYield),
                "Coroutine yield boundary has a protocol-independent semantic event");

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    state.reset();
    context.gc().clearAll(context.strings());
}

void testDebuggerNativeCallPauseRemainsPending(TestSuite& suite) {
    EngineContext context;
    DebugController& controller = context.globalState().enableDebugger();
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    Proto* proto = compileDebugChunk(services, "blocking_native()\nreturn 7\n", "=native_pause_pending");
    Function* chunk = createDebugFunction(services, state.get(), proto);
    Function* native = new Function(blockingNativeCall);
    services.gc.registerObject(native);
    state->setGlobal("blocking_native", Value(native));

    auto attached = controller.attachSession();
    DebugSession session = std::move(*attached);
    const bool configured = controller.configurationDone().has_value();
    gNativeCallEntered.store(false, std::memory_order_release);
    gReleaseNativeCall.store(false, std::memory_order_release);
    bool sawPending = false;
    bool stoppedAfterReturn = false;
    bool mixedStackVisible = false;

    std::thread control([&]() {
        if (!waitForFlag(gNativeCallEntered)) {
            gReleaseNativeCall.store(true, std::memory_order_release);
            return;
        }
        if (!controller.pause(DebugController::mainThreadId())) {
            gReleaseNativeCall.store(true, std::memory_order_release);
            return;
        }
        sawPending = controller.snapshot().state == DebugSessionState::PauseRequested;
        gReleaseNativeCall.store(true, std::memory_order_release);
        stoppedAfterReturn = waitForState(controller, DebugSessionState::Suspended);
        if (stoppedAfterReturn) {
            auto frames = controller.stackTrace(DebugController::mainThreadId(), 0, 100);
            mixedStackVisible = frames && frames->size() >= 2 && frames->front().native && !frames->at(1).native;
            (void)controller.continueExecution(DebugController::mainThreadId());
        }
    });

    VM::execute(services, state.get(), chunk);
    control.join();

    ASSERT_TRUE(suite, configured && sawPending,
                "Pause stays pending while control is inside a blocking native C++ function");
    ASSERT_TRUE(suite, stoppedAfterReturn, "Pending pause is confirmed at the function-return safepoint");
    ASSERT_TRUE(suite, mixedStackVisible, "Paused mixed stack exposes the active C frame above its Lua caller");
    ASSERT_TRUE(suite, state->top().isNumber() && state->top().asNumber() == 7,
                "Native-call pause resumes without re-entering or skipping bytecode");

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    state.reset();
    context.gc().clearAll(context.strings());
}

void testDebuggerSessionDetachPolicies(TestSuite& suite) {
    DebugController controller;
    auto firstAttach = controller.attachSession();
    ASSERT_TRUE(suite, firstAttach.has_value(), "First debugger session attaches");
    ASSERT_TRUE(suite, !controller.attachSession(), "A second simultaneous session is rejected");

    DebugSession first = std::move(*firstAttach);
    ASSERT_TRUE(suite, controller.configurationDone().has_value(), "First session starts execution");
    first.disconnect(DisconnectAction::ContinueExecution);
    ASSERT_FALSE(suite, first.attached(), "Explicit disconnect invalidates the session token");
    ASSERT_TRUE(suite, controller.snapshot().state == DebugSessionState::Detached,
                "Continue-on-disconnect leaves the VM detached rather than terminated");
    ASSERT_FALSE(suite, controller.terminateRequested(), "Continue-on-disconnect does not request termination");

    {
        auto secondAttach = controller.attachSession({}, DisconnectAction::TerminateExecution);
        ASSERT_TRUE(suite, secondAttach.has_value(), "Controller can be reattached after clean detach");
        DebugSession second = std::move(*secondAttach);
        ASSERT_TRUE(suite, second.attached(), "Replacement session is active");
    }
    ASSERT_TRUE(suite, controller.snapshot().state == DebugSessionState::Terminated,
                "RAII session destruction applies terminate-on-disconnect policy");
    ASSERT_TRUE(suite, controller.terminateRequested(), "Adapter exit can request VM cancellation without a callback");
}

void testDebuggerRuntimeTerminationErrors(TestSuite& suite) {
    DebugController controller;
    auto attached = controller.attachSession();
    DebugSession session = std::move(*attached);
    ASSERT_TRUE(suite, controller.configurationDone().has_value(), "Error-path fixture starts execution");

    controller.notifyProgramTerminated(DebugTerminationReason::RuntimeError,
                                       DebugError{DebugErrorCode::RuntimeFailure, "fixture runtime failure", false});
    const DebugSessionSnapshot failed = controller.snapshot();
    ASSERT_TRUE(suite, failed.terminationReason == DebugTerminationReason::RuntimeError,
                "Runtime failure has a distinct termination reason");
    ASSERT_TRUE(suite, failed.lastError && failed.lastError->code == DebugErrorCode::RuntimeFailure,
                "Last structured runtime error remains queryable");
    ASSERT_EQ(suite, Str("fixture runtime failure"), failed.lastError ? failed.lastError->message : Str{},
              "Last error message is copied into the lifecycle snapshot");
    ASSERT_TRUE(suite, !controller.pause(DebugController::mainThreadId()),
                "Terminal session rejects subsequent execution requests");
}

void testDebuggerRuntimeServicesFastPathAndHookIsolation(TestSuite& suite) {
    EngineContext context;
    RuntimeServices plainServices = context.services();
    ASSERT_TRUE(suite, plainServices.debugger == nullptr, "Debugger-disabled services keep a null fast-path pointer");

    UPtr<LuaState> state = LuaState::create(context);
    state->setApiDebugHook(noOpApiHook, HookMaskLine | HookMaskCall, 3);
    const ApiDebugHook hookBefore = state->getApiDebugHook();
    const u8 maskBefore = state->getDebugHookMask();
    const i32 countBefore = state->getDebugHookCount();

    DebugController& controller = context.globalState().enableDebugger();
    RuntimeServices debugServices = context.services();
    ASSERT_TRUE(suite, debugServices.debugger == &controller,
                "RuntimeServices exposes the context-owned optional DebugController");
    auto attached = controller.attachSession();
    DebugSession session = std::move(*attached);
    session.disconnect(DisconnectAction::ContinueExecution);

    ASSERT_TRUE(suite, state->getApiDebugHook() == hookBefore,
                "Debugger attachment does not replace lua_sethook callback");
    ASSERT_EQ(suite, maskBefore, state->getDebugHookMask(), "Debugger attachment preserves user hook mask");
    ASSERT_EQ(suite, countBefore, state->getDebugHookCount(), "Debugger attachment preserves user hook count");

    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    ASSERT_TRUE(suite, context.services().debugger == nullptr, "Disabling debugger restores the null fast path");
    ASSERT_FALSE(suite, session.attached(), "Controller destruction leaves no dangling session attachment");

    state->setApiDebugHook(nullptr, 0, 0);
    state.reset();
    context.gc().clearAll(context.strings());
}

void testDebuggerStateAndCoroutineRegistry(TestSuite& suite) {
    EngineContext context;
    UPtr<LuaState> mainState = LuaState::create(context);
    DebugController& controller = context.globalState().enableDebugger();
    Ptr<RecordingDebugSink> sink = makePtr<RecordingDebugSink>();
    auto attached = controller.attachSession(sink);
    DebugSession session = std::move(*attached);

    auto initialStates = controller.states();
    ASSERT_TRUE(suite, initialStates && initialStates->size() == 1 && initialStates->front().id == StateId{1} &&
                           initialStates->front().threadId == ThreadId{1} && initialStates->front().selected,
                "Enabling the debugger registers an existing root state with stable state and thread IDs");

    LuaState* coroutine = LuaState::newThread(mainState.get());
    auto withCoroutine = controller.states();
    auto threads = controller.threads();
    ASSERT_TRUE(suite, coroutine != nullptr && withCoroutine && withCoroutine->size() == 2 && threads &&
                           threads->size() == 2,
                "New coroutine states are registered without enumerating raw GC objects");
    const DebugState child = withCoroutine && withCoroutine->size() == 2 ? (*withCoroutine)[1] : DebugState{};
    ASSERT_TRUE(suite, child.id.valid() && child.threadId.valid() && child.name.find("coroutine") != Str::npos,
                "Coroutine registry exposes copied labels and opaque IDs instead of addresses");
    ASSERT_TRUE(suite, controller.selectState(child.id).has_value(), "A remote client can select a registered state");

    LuaState::destroyState(coroutine);
    auto afterExit = controller.states();
    ASSERT_TRUE(suite, afterExit && afterExit->size() == 1 && afterExit->front().selected,
                "Destroying a selected coroutine removes it and selects a live fallback state");
    {
        std::lock_guard lock(sink->mutex);
        ASSERT_TRUE(suite, sink->executionUnits.size() == 2 && sink->executionUnits[0].second &&
                               !sink->executionUnits[1].second &&
                               sink->executionUnits[0].first.id == sink->executionUnits[1].first.id,
                    "Coroutine start and exit publish paired lifecycle events with one stable ID");
    }

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    mainState.reset();
    context.gc().clearAll(context.strings());
}

void testDebuggerSourceSteppingModes(TestSuite& suite) {
    constexpr StrView source = "local function inner(value)\n"
                               "    local doubled = value * 2\n"
                               "    return doubled\n"
                               "end\n"
                               "\n"
                               "local first = 3\n"
                               "local result = inner(first)\n"
                               "local final = result + 1\n"
                               "return final\n";
    struct Case {
        DebugStepMode mode;
        bool stepOutAfterEntering;
        i32 expectedLine;
    };
    const std::array cases{Case{DebugStepMode::In, false, 2}, Case{DebugStepMode::Over, false, 8},
                           Case{DebugStepMode::In, true, 8}};

    for (const Case& testCase : cases) {
        EngineContext context;
        DebugController& controller = context.globalState().enableDebugger();
        RuntimeServices services = context.services();
        UPtr<LuaState> state = LuaState::create(context);
        Proto* proto = compileDebugChunk(services, source, "@debugger/step_state.lua");
        Function* function = createDebugFunction(services, state.get(), proto);
        const SourceId sourceId = controller.registerFilePath("debugger/step_state.lua");
        const std::array breakpoint{SourceBreakpoint{7}};
        (void)controller.setBreakpoints(sourceId, breakpoint);
        auto attached = controller.attachSession();
        DebugSession session = std::move(*attached);
        (void)controller.configurationDone();

        bool completed = false;
        bool reasonsWereSteps = true;
        i32 finalLine = 0;
        std::thread control([&]() {
            PauseGeneration handled;
            usize stops = 0;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
            while (std::chrono::steady_clock::now() < deadline) {
                const DebugSessionSnapshot snapshot = controller.snapshot();
                if (snapshot.state != DebugSessionState::Suspended || snapshot.pauseGeneration == handled) {
                    std::this_thread::yield();
                    continue;
                }
                handled = snapshot.pauseGeneration;
                ++stops;
                if (stops == 1) {
                    (void)controller.setBreakpoints(sourceId, {});
                    (void)controller.stepExecution(ThreadId{1}, testCase.mode);
                    continue;
                }

                reasonsWereSteps = reasonsWereSteps && snapshot.stopReason == DebugStopReason::Step;
                auto frames = controller.stackTrace(ThreadId{1}, 0, 20);
                if (!frames || frames->empty()) {
                    break;
                }
                if (testCase.stepOutAfterEntering && stops == 2) {
                    reasonsWereSteps = reasonsWereSteps && frames->front().location.line == 2;
                    (void)controller.stepExecution(ThreadId{1}, DebugStepMode::Out);
                    continue;
                }
                finalLine = frames->front().location.line;
                completed = controller.continueExecution(ThreadId{1}).has_value();
                return;
            }
            (void)controller.terminateExecution();
        });

        bool executed = true;
        try {
            VM::execute(services, state.get(), function);
        } catch (const RuntimeError&) {
            executed = false;
        }
        control.join();

        ASSERT_TRUE(suite, completed && executed && reasonsWereSteps && finalLine == testCase.expectedLine,
                    "Table-driven Step In, Step Over, and Step Out stop once at the expected source frame");
        session.disconnect(DisconnectAction::ContinueExecution);
        context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
        state.reset();
        context.gc().clearAll(context.strings());
    }
}

void registerDebuggerRuntimeTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Lifecycle State Machine", testDebuggerRuntimeStateMachine);
    registry.registerTest(kSuiteName, "Session Detach Policies", testDebuggerSessionDetachPolicies);
    registry.registerTest(kSuiteName, "Termination Error Snapshot", testDebuggerRuntimeTerminationErrors);
    registry.registerTest(kSuiteName, "Runtime Services And Hook Isolation",
                          testDebuggerRuntimeServicesFastPathAndHookIsolation);
    registry.registerTest(kSuiteName, "State And Coroutine Registry", testDebuggerStateAndCoroutineRegistry);
    registry.registerTest(kSuiteName, "Instruction Safepoint Exactly Once",
                          testDebuggerSafepointExecutesCurrentInstructionOnce);
    registry.registerTest(kSuiteName, "Safepoint Pause Continue Stress", testDebuggerSafepointStress);
    registry.registerTest(kSuiteName, "Safepoint Termination Races", testDebuggerSafepointTerminationRaces);
    registry.registerTest(kSuiteName, "VM Safepoint And Semantic Integration",
                          testDebuggerVmSafepointAndSemanticIntegration);
    registry.registerTest(kSuiteName, "Semantic Events And Exception Boundary",
                          testDebuggerSemanticEventsAndExceptionBoundary);
    registry.registerTest(kSuiteName, "Native Call Pause Pending", testDebuggerNativeCallPauseRemainsPending);
    registry.registerTest(kSuiteName, "Source Stepping Modes", testDebuggerSourceSteppingModes);
}
