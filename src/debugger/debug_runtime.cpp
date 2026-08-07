/**
 * @file debug_runtime.cpp
 * @brief Protocol-independent debugger lifecycle implementation.
 */

#include "debugger/debug_runtime.hpp"
#include "debugger/stack_inspector.hpp"

#include "core/function.hpp"
#include "vm/state/call_info.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/stack.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <utility>

namespace Lua::Debugger {

namespace {

class DebugOwnerCommand {
public:
    virtual ~DebugOwnerCommand() = default;
    virtual void execute(StackInspector& inspector) noexcept = 0;
    virtual void cancel(DebugError error) noexcept = 0;
};

template <typename T> class TypedDebugOwnerCommand final : public DebugOwnerCommand {
public:
    explicit TypedDebugOwnerCommand(Func<DebugResult<T>(StackInspector&)> operation)
        : operation_(std::move(operation)) {}

    [[nodiscard]] std::future<DebugResult<T>> future() {
        return promise_.get_future();
    }

    void execute(StackInspector& inspector) noexcept override {
        try {
            promise_.set_value(operation_(inspector));
        } catch (const std::exception& error) {
            promise_.set_value(std::unexpected(
                DebugError{DebugErrorCode::RuntimeFailure, Str("debug inspection failed: ") + error.what()}));
        } catch (...) {
            promise_.set_value(std::unexpected(DebugError{DebugErrorCode::RuntimeFailure, "debug inspection failed"}));
        }
    }

    void cancel(DebugError error) noexcept override {
        try {
            promise_.set_value(std::unexpected(std::move(error)));
        } catch (...) {
        }
    }

private:
    Func<DebugResult<T>(StackInspector&)> operation_;
    std::promise<DebugResult<T>> promise_;
};

} // namespace

struct StepExecutionState {
    Opt<DebugStepMode> pendingMode;
    Opt<DebugStepMode> mode;
    ThreadId thread;
    const LuaState* originState = nullptr;
    usize originDepth = 0;
    usize originFunctionSlot = 0;
    const Function* originFunction = nullptr;
    const Proto* originProto = nullptr;
    usize originPc = 0;
    i32 originLine = 0;
    usize inspectedInstructions = 0;
};

struct RegisteredDebugState {
    StateId id;
    ThreadId threadId;
    LuaState* state = nullptr;
    Str name;
    Str label;
    DebugThreadState status = DebugThreadState::Running;
};

struct DebugControllerState {
    DebugControllerState(DebugResourceLimits limits, std::atomic<u8>& flags)
        : inspector(breakpoints, limits), maxStates(limits.maxStates),
          maxLogMessageLength(limits.maxLogMessageLength),
          maxLogMessagesPerSecond(limits.maxLogMessagesPerSecond), instructionFlags(&flags) {}

    mutable std::mutex mutex;
    std::condition_variable condition;
    std::atomic<bool> pauseRequested = false;
    std::atomic<bool> terminateRequested = false;
    DebugSessionState state = DebugSessionState::Detached;
    Opt<DebugStopReason> stopReason;
    Opt<DebugTerminationReason> terminationReason;
    Opt<DebugError> lastError;
    PauseGeneration pauseGeneration;
    WPtr<IDebugEventSink> sink;
    u64 activeSessionId = 0;
    u64 nextSessionId = 1;
    BreakpointManager breakpoints;
    StackInspector inspector;
    std::deque<UPtr<DebugOwnerCommand>> commands;
    std::atomic<u64> breakpointEpoch = 1;
    u64 ownerBreakpointEpoch = 0;
    const Proto* suppressedProto = nullptr;
    i32 suppressedLine = 0;
    usize suppressedLastPc = 0;
    bool suppressingBreakpoint = false;
    StepExecutionState step;
    bool breakOnAllExceptions = false;
    bool exceptionPropagationActive = false;
    Opt<DebugExceptionInfo> exception;
    Opt<Value> exceptionValue;
    HashMap<const LuaState*, RegisteredDebugState> statesByPointer;
    HashMap<u64, const LuaState*> statesById;
    HashMap<u64, const LuaState*> statesByThread;
    StateId selectedState;
    ThreadId pausedThread{1};
    u64 nextStateId = 1;
    u64 nextThreadId = 1;
    usize maxStates = 256;
    usize maxLogMessageLength = 4096;
    usize maxLogMessagesPerSecond = 100;
    std::chrono::steady_clock::time_point logWindowStart = std::chrono::steady_clock::now();
    usize logMessagesInWindow = 0;
    usize droppedLogMessages = 0;
    u64 totalDroppedLogMessages = 0;
    DebugWritePolicy writePolicy;
    std::atomic<u8>* instructionFlags = nullptr;
};

namespace {

DebugError invalidState(Str message) {
    return {DebugErrorCode::InvalidState, std::move(message)};
}

void refreshInstructionFlags(DebugControllerState& state) noexcept {
    constexpr u8 kPause = 1U << 0U;
    constexpr u8 kTerminate = 1U << 1U;
    constexpr u8 kBreakpoint = 1U << 2U;
    constexpr u8 kStep = 1U << 3U;
    u8 flags = 0;
    if (state.pauseRequested.load(std::memory_order_relaxed)) {
        flags |= kPause;
    }
    if (state.terminateRequested.load(std::memory_order_relaxed)) {
        flags |= kTerminate;
    }
    if (state.breakpoints.hasBreakpoints()) {
        flags |= kBreakpoint;
    }
    if (state.step.pendingMode || state.step.mode) {
        flags |= kStep;
    }
    state.instructionFlags->store(flags, std::memory_order_release);
}

void clearStepLocked(DebugControllerState& state) noexcept {
    state.step = {};
}

void clearExceptionLocked(DebugControllerState& state) noexcept {
    state.exceptionPropagationActive = false;
    state.exception.reset();
    state.exceptionValue.reset();
}

Str exceptionId(DebugExceptionCategory category) {
    switch (category) {
    case DebugExceptionCategory::RuntimeError:
        return "lua.runtimeError";
    case DebugExceptionCategory::ResourceError:
        return "lua.resourceError";
    case DebugExceptionCategory::HostCancellation:
        return "host.cancelled";
    }
    return "lua.runtimeError";
}

DebugState snapshotStateLocked(const DebugControllerState& state, const RegisteredDebugState& registered) {
    return {registered.id, registered.threadId, registered.name, registered.label, registered.status,
            registered.id == state.selectedState};
}

RegisteredDebugState* findThreadLocked(DebugControllerState& state, ThreadId thread) noexcept {
    const auto pointer = state.statesByThread.find(thread.value());
    if (pointer == state.statesByThread.end()) {
        return nullptr;
    }
    const auto registered = state.statesByPointer.find(pointer->second);
    return registered == state.statesByPointer.end() ? nullptr : &registered->second;
}

const RegisteredDebugState* findThreadLocked(const DebugControllerState& state, ThreadId thread) noexcept {
    const auto pointer = state.statesByThread.find(thread.value());
    if (pointer == state.statesByThread.end()) {
        return nullptr;
    }
    const auto registered = state.statesByPointer.find(pointer->second);
    return registered == state.statesByPointer.end() ? nullptr : &registered->second;
}

RegisteredDebugState* findStateLocked(DebugControllerState& state, StateId id) noexcept {
    const auto pointer = state.statesById.find(id.value());
    if (pointer == state.statesById.end()) {
        return nullptr;
    }
    const auto registered = state.statesByPointer.find(pointer->second);
    return registered == state.statesByPointer.end() ? nullptr : &registered->second;
}

ThreadId pausedThreadLocked(const DebugControllerState& state, const LuaState* pausedState) noexcept {
    const auto found = state.statesByPointer.find(pausedState);
    return found == state.statesByPointer.end() ? DebugController::mainThreadId() : found->second.threadId;
}

Str pausedThreadNameLocked(const DebugControllerState& state, const LuaState* pausedState) {
    const auto found = state.statesByPointer.find(pausedState);
    return found == state.statesByPointer.end() ? Str("main") : found->second.name;
}

void captureStepOriginLocked(DebugControllerState& state, LuaState& pausedState) noexcept {
    state.step.mode = state.step.pendingMode;
    state.step.pendingMode.reset();
    state.step.thread = pausedThreadLocked(state, &pausedState);
    state.step.originState = &pausedState;
    state.step.inspectedInstructions = 0;

    const usize callIndex = pausedState.getCurrentCI();
    state.step.originDepth = callIndex;
    if (callIndex >= pausedState.getCallStack().size()) {
        return;
    }
    const CallInfo& call = pausedState.getCallStack()[callIndex];
    state.step.originFunctionSlot = call.func;
    Stack& stack = pausedState.getStack();
    if (call.func >= stack.size() || !stack[call.func].isFunction()) {
        return;
    }
    Function* function = stack[call.func].asFunction();
    state.step.originFunction = function;
    if (function == nullptr || function->isCFunction() || function->getProto() == nullptr) {
        return;
    }
    const Proto* proto = function->getProto();
    state.step.originProto = proto;
    const auto code = proto->getInstructionSpan();
    if (call.savedpc != nullptr && !code.empty() && call.savedpc > code.data() &&
        call.savedpc <= code.data() + code.size()) {
        state.step.originPc = static_cast<usize>(call.savedpc - code.data() - 1);
    }
    if (state.step.originPc < proto->getLineInfo().size()) {
        state.step.originLine = proto->getLine(state.step.originPc);
    }
}

bool shouldStopStepLocked(DebugControllerState& state, LuaState& pausedState, const Proto& proto, usize pc) noexcept {
    if (!state.step.mode) {
        return false;
    }

    constexpr usize kInstructionFallback = 100000;
    ++state.step.inspectedInstructions;
    const usize depth = pausedState.getCurrentCI();
    const CallInfo& call = pausedState.getCurrentCallInfo();
    const Stack& stack = pausedState.getStack();
    const Function* function =
        call.func < stack.size() && stack[call.func].isFunction() ? stack[call.func].asFunction() : nullptr;
    const i32 line = pc < proto.getLineInfo().size() ? proto.getLine(pc) : 0;
    const bool visible = line > 0;
    const bool sameState = &pausedState == state.step.originState;
    const bool sameFrame = sameState && depth == state.step.originDepth && call.func == state.step.originFunctionSlot &&
                           function == state.step.originFunction && &proto == state.step.originProto;
    const bool differentVisiblePosition =
        visible && (!sameFrame || state.step.originLine <= 0 || line != state.step.originLine);

    bool stop = false;
    switch (*state.step.mode) {
    case DebugStepMode::In:
        stop = differentVisiblePosition;
        break;
    case DebugStepMode::Over:
        stop = sameState && ((depth < state.step.originDepth && visible) ||
                             (depth == state.step.originDepth && differentVisiblePosition));
        break;
    case DebugStepMode::Out:
        stop = sameState && ((depth < state.step.originDepth && visible) ||
                             (depth == state.step.originDepth && !sameFrame && visible));
        break;
    }

    if (!stop && state.step.inspectedInstructions >= kInstructionFallback) {
        stop = pc != state.step.originPc || !sameFrame;
    }
    if (stop) {
        state.step.mode.reset();
        refreshInstructionFlags(state);
    }
    return stop;
}

DebugSessionSnapshot snapshotLocked(const DebugControllerState& state) {
    return {state.state,
            state.stopReason,
            state.terminationReason,
            state.lastError,
            state.pauseGeneration,
            state.activeSessionId != 0,
            state.pauseRequested.load(std::memory_order_acquire),
            state.terminateRequested.load(std::memory_order_acquire),
            state.pausedThread,
            state.totalDroppedLogMessages};
}

void publish(const Ptr<IDebugEventSink>& sink, const DebugSessionSnapshot& snapshot) noexcept {
    if (sink == nullptr) {
        return;
    }
    try {
        sink->onDebugStateChanged(snapshot);
    } catch (...) {
        // A client callback must not unwind into the VM or destroy lifecycle state.
    }
}

struct EvaluatedBreakpointBehavior {
    bool conditionMatched = true;
    bool conditionError = false;
    bool logError = false;
    Opt<Str> logOutput;
    Opt<DebugError> error;
};

EvaluatedBreakpointBehavior evaluateBreakpointBehavior(DebugControllerState& state, LuaState& luaState,
                                                        const BreakpointBehavior& behavior) {
    EvaluatedBreakpointBehavior result;
    if (behavior.condition.empty() && behavior.logMessage.empty()) {
        return result;
    }

    ThreadId thread;
    Str threadName;
    PauseGeneration temporaryGeneration;
    {
        std::lock_guard lock(state.mutex);
        thread = pausedThreadLocked(state, &luaState);
        threadName = pausedThreadNameLocked(state, &luaState);
        temporaryGeneration = PauseGeneration{state.pauseGeneration.value() + 1};
    }
    state.inspector.beginPause(luaState, temporaryGeneration, thread, threadName);
    struct EndTemporaryInspection {
        StackInspector& inspector;
        ~EndTemporaryInspection() {
            inspector.endPause();
        }
    } end{state.inspector};

    auto frames = state.inspector.stackTrace(thread, 0, 1);
    if (!frames || frames->empty()) {
        result.conditionError = !behavior.condition.empty();
        result.logError = !behavior.logMessage.empty();
        result.error = frames ? DebugError{DebugErrorCode::InvalidReference, "breakpoint has no live Lua frame"}
                              : frames.error();
        return result;
    }
    const FrameId frame = frames->front().id;
    const auto evaluate = [&](StrView expression) { return state.inspector.evaluate(frame, expression); };

    if (!behavior.condition.empty()) {
        auto condition = evaluate(behavior.condition);
        if (!condition) {
            result.conditionError = true;
            result.error = condition.error();
            return result;
        }
        result.conditionMatched = condition->type != "nil" &&
                                  !(condition->type == "boolean" && condition->value == "false");
        if (!result.conditionMatched) {
            return result;
        }
    }

    if (behavior.logMessage.empty()) {
        return result;
    }
    Str rendered;
    rendered.reserve(std::min(behavior.logMessage.size(), state.maxLogMessageLength));
    for (usize index = 0; index < behavior.logMessage.size();) {
        const char character = behavior.logMessage[index];
        if (character == '{' && index + 1 < behavior.logMessage.size() && behavior.logMessage[index + 1] == '{') {
            rendered.push_back('{');
            index += 2;
        } else if (character == '}' && index + 1 < behavior.logMessage.size() &&
                   behavior.logMessage[index + 1] == '}') {
            rendered.push_back('}');
            index += 2;
        } else if (character == '{') {
            const usize close = behavior.logMessage.find('}', index + 1);
            if (close == Str::npos) {
                result.logError = true;
                result.error = DebugError{DebugErrorCode::Unsupported, "log point has an unmatched '{'"};
                return result;
            }
            StrView expression(behavior.logMessage.data() + index + 1, close - index - 1);
            while (!expression.empty() && std::isspace(static_cast<unsigned char>(expression.front())) != 0) {
                expression.remove_prefix(1);
            }
            while (!expression.empty() && std::isspace(static_cast<unsigned char>(expression.back())) != 0) {
                expression.remove_suffix(1);
            }
            if (expression.empty()) {
                result.logError = true;
                result.error = DebugError{DebugErrorCode::Unsupported, "log point interpolation is empty"};
                return result;
            }
            auto value = evaluate(expression);
            if (!value) {
                result.logError = true;
                result.error = value.error();
                return result;
            }
            rendered += value->value;
            index = close + 1;
        } else if (character == '}') {
            result.logError = true;
            result.error = DebugError{DebugErrorCode::Unsupported, "log point has an unmatched '}'"};
            return result;
        } else {
            rendered.push_back(character);
            ++index;
        }
        if (rendered.size() > state.maxLogMessageLength) {
            result.logError = true;
            result.error = DebugError{DebugErrorCode::ResourceLimit,
                                      "rendered log point exceeds the configured byte limit"};
            return result;
        }
    }
    rendered.push_back('\n');
    result.logOutput = std::move(rendered);
    return result;
}

void publishDebugOutput(DebugControllerState& state, Str message) noexcept {
    Ptr<IDebugEventSink> sink;
    Opt<Str> suppressed;
    bool allowed = false;
    {
        std::lock_guard lock(state.mutex);
        const auto now = std::chrono::steady_clock::now();
        if (now - state.logWindowStart >= std::chrono::seconds(1)) {
            if (state.droppedLogMessages != 0) {
                suppressed = "[YanLua] suppressed " + std::to_string(state.droppedLogMessages) +
                             " log point messages\n";
            }
            state.logWindowStart = now;
            state.logMessagesInWindow = 0;
            state.droppedLogMessages = 0;
        }
        if (state.logMessagesInWindow < state.maxLogMessagesPerSecond) {
            ++state.logMessagesInWindow;
            allowed = true;
            sink = state.sink.lock();
        } else {
            ++state.droppedLogMessages;
            ++state.totalDroppedLogMessages;
        }
    }
    if (sink == nullptr) {
        return;
    }
    try {
        if (suppressed) {
            sink->onDebugOutput(*suppressed, DebugOutputCategory::Console);
        }
        if (allowed) {
            sink->onDebugOutput(message, DebugOutputCategory::Console);
        }
    } catch (...) {
    }
}

void cancelCommandsLocked(DebugControllerState& state, const DebugError& error) noexcept {
    while (!state.commands.empty()) {
        UPtr<DebugOwnerCommand> command = std::move(state.commands.front());
        state.commands.pop_front();
        command->cancel(error);
    }
}

void detachSession(const Ptr<DebugControllerState>& state, u64 sessionId, DisconnectAction action) noexcept {
    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot snapshot;
    {
        std::lock_guard lock(state->mutex);
        if (sessionId == 0 || state->activeSessionId != sessionId) {
            return;
        }

        sink = state->sink.lock();
        state->activeSessionId = 0;
        state->sink.reset();
        state->pauseRequested.store(false, std::memory_order_release);
        state->stopReason.reset();
        clearStepLocked(*state);
        clearExceptionLocked(*state);
        state->breakpoints.clearBreakpoints();
        cancelCommandsLocked(
            *state, DebugError{DebugErrorCode::StaleReference, "debug inspection cancelled because the pause ended"});
        state->breakpointEpoch.fetch_add(1, std::memory_order_release);
        if (action == DisconnectAction::TerminateExecution) {
            state->terminateRequested.store(true, std::memory_order_release);
            state->terminationReason = DebugTerminationReason::Requested;
            state->state = DebugSessionState::Terminated;
        } else {
            state->terminateRequested.store(false, std::memory_order_release);
            state->terminationReason.reset();
            state->state = DebugSessionState::Detached;
        }
        refreshInstructionFlags(*state);
        snapshot = snapshotLocked(*state);
    }
    state->condition.notify_all();
    publish(sink, snapshot);
}

DebugSafepointResult waitForSafepointRequest(const Ptr<DebugControllerState>& state,
                                             Opt<DebugStopReason> forcedReason = std::nullopt,
                                             LuaState* pausedState = nullptr) {
    if (state->terminateRequested.load(std::memory_order_acquire)) {
        return DebugSafepointResult::TerminateExecution;
    }
    if (!forcedReason && !state->pauseRequested.load(std::memory_order_acquire)) {
        return DebugSafepointResult::ContinueExecution;
    }

    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot suspended;
    std::unique_lock lock(state->mutex);
    if (state->terminateRequested.load(std::memory_order_acquire) || state->state == DebugSessionState::Terminated) {
        return DebugSafepointResult::TerminateExecution;
    }
    if (state->activeSessionId == 0 || (state->state != DebugSessionState::PauseRequested &&
                                        !(forcedReason && state->state == DebugSessionState::Running))) {
        return DebugSafepointResult::ContinueExecution;
    }

    state->pauseRequested.store(false, std::memory_order_release);
    refreshInstructionFlags(*state);
    state->pauseGeneration = PauseGeneration{state->pauseGeneration.value() + 1};
    if (state->state == DebugSessionState::PauseRequested) {
        state->stopReason = DebugStopReason::Pause;
    } else {
        state->stopReason = forcedReason;
    }
    state->state = DebugSessionState::Suspended;
    if (pausedState != nullptr) {
        state->pausedThread = pausedThreadLocked(*state, pausedState);
        if (RegisteredDebugState* registered = findThreadLocked(*state, state->pausedThread)) {
            registered->status = DebugThreadState::Paused;
            if (!state->selectedState.valid()) {
                state->selectedState = registered->id;
            }
        }
        const Value* exceptionValue =
            state->stopReason == DebugStopReason::Exception && state->exceptionValue ? &*state->exceptionValue : nullptr;
        state->inspector.beginPause(*pausedState, state->pauseGeneration, state->pausedThread,
                                    pausedThreadNameLocked(*state, pausedState), exceptionValue);
    }
    sink = state->sink.lock();
    suspended = snapshotLocked(*state);
    lock.unlock();
    publish(sink, suspended);
    lock.lock();

    for (;;) {
        state->condition.wait(lock, [&]() {
            return state->state != DebugSessionState::Suspended || !state->commands.empty() ||
                   state->terminateRequested.load(std::memory_order_acquire);
        });
        if (state->state != DebugSessionState::Suspended || state->terminateRequested.load(std::memory_order_acquire)) {
            break;
        }

        UPtr<DebugOwnerCommand> command = std::move(state->commands.front());
        state->commands.pop_front();
        lock.unlock();
        command->execute(state->inspector);
        lock.lock();
    }

    if (state->state == DebugSessionState::ResumeRequested && state->step.pendingMode && pausedState != nullptr) {
        captureStepOriginLocked(*state, *pausedState);
        refreshInstructionFlags(*state);
    }

    lock.unlock();
    state->inspector.endPause();
    lock.lock();
    if (RegisteredDebugState* registered = findThreadLocked(*state, state->pausedThread)) {
        registered->status = DebugThreadState::Running;
    }

    if (state->terminateRequested.load(std::memory_order_acquire) || state->state == DebugSessionState::Terminated) {
        return DebugSafepointResult::TerminateExecution;
    }
    if (state->state == DebugSessionState::ResumeRequested) {
        state->state = DebugSessionState::Running;
        sink = state->sink.lock();
        const DebugSessionSnapshot running = snapshotLocked(*state);
        lock.unlock();
        publish(sink, running);
    }
    return DebugSafepointResult::ContinueExecution;
}

template <typename T>
DebugResult<T> enqueueOwnerCommand(const Ptr<DebugControllerState>& state,
                                   Func<DebugResult<T>(StackInspector&)> operation) {
    auto command = makeUnique<TypedDebugOwnerCommand<T>>(std::move(operation));
    std::future<DebugResult<T>> result = command->future();
    {
        std::lock_guard lock(state->mutex);
        if (state->activeSessionId == 0 || state->state != DebugSessionState::Suspended) {
            return std::unexpected(
                DebugError{DebugErrorCode::InvalidState, "debug inspection requires a suspended session"});
        }
        state->commands.push_back(std::move(command));
    }
    state->condition.notify_all();

    if (result.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        return std::unexpected(DebugError{DebugErrorCode::Timeout, "debug owner-thread command timed out", true});
    }
    return result.get();
}

} // namespace

DebugSession::DebugSession(WPtr<DebugControllerState> state, u64 sessionId, DisconnectAction disconnectAction) noexcept
    : state_(std::move(state)), sessionId_(sessionId), disconnectAction_(disconnectAction) {}

DebugSession::~DebugSession() {
    disconnect(disconnectAction_);
}

DebugSession::DebugSession(DebugSession&& other) noexcept
    : state_(std::move(other.state_)), sessionId_(std::exchange(other.sessionId_, 0)),
      disconnectAction_(other.disconnectAction_), proxyAttached_(std::move(other.proxyAttached_)),
      proxyDisconnect_(std::move(other.proxyDisconnect_)) {}

DebugSession& DebugSession::operator=(DebugSession&& other) noexcept {
    if (this != &other) {
        disconnect(disconnectAction_);
        state_ = std::move(other.state_);
        sessionId_ = std::exchange(other.sessionId_, 0);
        disconnectAction_ = other.disconnectAction_;
        proxyAttached_ = std::move(other.proxyAttached_);
        proxyDisconnect_ = std::move(other.proxyDisconnect_);
    }
    return *this;
}

bool DebugSession::attached() const noexcept {
    if (proxyAttached_) {
        try {
            return proxyAttached_();
        } catch (...) {
            return false;
        }
    }
    const Ptr<DebugControllerState> state = state_.lock();
    if (state == nullptr || sessionId_ == 0) {
        return false;
    }
    std::lock_guard lock(state->mutex);
    return state->activeSessionId == sessionId_;
}

void DebugSession::disconnect(DisconnectAction action) noexcept {
    if (proxyDisconnect_) {
        Func<void(DisconnectAction)> disconnect = std::move(proxyDisconnect_);
        proxyAttached_ = {};
        try {
            disconnect(action);
        } catch (...) {
        }
        return;
    }
    const Ptr<DebugControllerState> state = state_.lock();
    if (state != nullptr && sessionId_ != 0) {
        detachSession(state, sessionId_, action);
    }
    sessionId_ = 0;
    state_.reset();
}

DebugSession DebugSession::proxy(Func<bool()> attached, Func<void(DisconnectAction)> disconnect,
                                 DisconnectAction disconnectAction) {
    DebugSession result;
    result.disconnectAction_ = disconnectAction;
    result.proxyAttached_ = std::move(attached);
    result.proxyDisconnect_ = std::move(disconnect);
    return result;
}

DebugController::DebugController(DebugResourceLimits limits)
    : state_(makePtr<DebugControllerState>(limits, instructionFlags_)) {}

DebugController::~DebugController() {
    shutdown(DisconnectAction::ContinueExecution);
}

DebugResult<DebugSession> DebugController::attachSession(Ptr<IDebugEventSink> sink, DisconnectAction disconnectAction) {
    u64 sessionId = 0;
    DebugSessionSnapshot current;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId != 0) {
            return std::unexpected(invalidState("a debugger session is already attached"));
        }

        sessionId = state_->nextSessionId++;
        state_->activeSessionId = sessionId;
        state_->sink = sink;
        state_->pauseRequested.store(false, std::memory_order_release);
        state_->terminateRequested.store(false, std::memory_order_release);
        state_->stopReason.reset();
        state_->terminationReason.reset();
        state_->lastError.reset();
        clearStepLocked(*state_);
        clearExceptionLocked(*state_);
        state_->state = DebugSessionState::Starting;
        state_->breakpointEpoch.fetch_add(1, std::memory_order_release);
        refreshInstructionFlags(*state_);
        current = snapshotLocked(*state_);
    }
    publish(sink, current);
    return DebugSession{state_, sessionId, disconnectAction};
}

DebugResult<void> DebugController::configurationDone() {
    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot current;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId == 0 || state_->state != DebugSessionState::Starting) {
            return std::unexpected(invalidState("configurationDone requires a starting debug session"));
        }
        state_->state = DebugSessionState::Running;
        sink = state_->sink.lock();
        current = snapshotLocked(*state_);
    }
    publish(sink, current);
    state_->condition.notify_all();
    return {};
}

DebugResult<void> DebugController::pause(ThreadId thread) {
    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot current;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId == 0 || state_->state != DebugSessionState::Running) {
            return std::unexpected(invalidState("pause requires a running debug session"));
        }
        if ((!state_->statesByPointer.empty() && findThreadLocked(*state_, thread) == nullptr) ||
            (state_->statesByPointer.empty() && thread != mainThreadId())) {
            return std::unexpected(DebugError{DebugErrorCode::InvalidReference, "unknown debug thread"});
        }
        state_->pauseRequested.store(true, std::memory_order_release);
        state_->state = DebugSessionState::PauseRequested;
        refreshInstructionFlags(*state_);
        sink = state_->sink.lock();
        current = snapshotLocked(*state_);
    }
    publish(sink, current);
    state_->condition.notify_all();
    return {};
}

DebugResult<void> DebugController::continueExecution(ThreadId thread) {
    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot current;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId == 0 || state_->state != DebugSessionState::Suspended) {
            return std::unexpected(invalidState("continue requires a suspended debug session"));
        }
        if (thread != state_->pausedThread) {
            return std::unexpected(DebugError{DebugErrorCode::InvalidReference, "thread is not the paused execution unit"});
        }
        state_->pauseRequested.store(false, std::memory_order_release);
        state_->stopReason.reset();
        clearStepLocked(*state_);
        cancelCommandsLocked(*state_, DebugError{DebugErrorCode::StaleReference,
                                                 "debug inspection cancelled because execution resumed"});
        state_->state = DebugSessionState::ResumeRequested;
        refreshInstructionFlags(*state_);
        sink = state_->sink.lock();
        current = snapshotLocked(*state_);
    }
    publish(sink, current);
    state_->condition.notify_all();
    return {};
}

DebugResult<void> DebugController::stepExecution(ThreadId thread, DebugStepMode mode) {
    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot current;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId == 0 || state_->state != DebugSessionState::Suspended) {
            return std::unexpected(invalidState("step requires a suspended debug session"));
        }
        if (thread != state_->pausedThread) {
            return std::unexpected(DebugError{DebugErrorCode::InvalidReference, "thread is not the paused execution unit"});
        }
        state_->pauseRequested.store(false, std::memory_order_release);
        state_->stopReason.reset();
        cancelCommandsLocked(*state_, DebugError{DebugErrorCode::StaleReference,
                                                 "debug inspection cancelled because execution stepped"});
        clearStepLocked(*state_);
        state_->step.pendingMode = mode;
        state_->state = DebugSessionState::ResumeRequested;
        refreshInstructionFlags(*state_);
        sink = state_->sink.lock();
        current = snapshotLocked(*state_);
    }
    publish(sink, current);
    state_->condition.notify_all();
    return {};
}

DebugResult<void> DebugController::setExceptionBreakpoints(bool breakOnAll) {
    std::lock_guard lock(state_->mutex);
    if (state_->activeSessionId == 0 || state_->state == DebugSessionState::Terminated) {
        return std::unexpected(invalidState("exception breakpoints require an active debug session"));
    }
    state_->breakOnAllExceptions = breakOnAll;
    if (!breakOnAll) {
        clearExceptionLocked(*state_);
    }
    return {};
}

DebugResult<DebugExceptionInfo> DebugController::exceptionInfo(ThreadId thread) {
    std::lock_guard lock(state_->mutex);
    if (thread != state_->pausedThread) {
        return std::unexpected(DebugError{DebugErrorCode::InvalidReference, "thread is not the paused execution unit"});
    }
    if (state_->state != DebugSessionState::Suspended || state_->stopReason != DebugStopReason::Exception ||
        !state_->exception) {
        return std::unexpected(invalidState("exceptionInfo requires an exception suspension"));
    }
    return *state_->exception;
}

DebugResult<void> DebugController::terminateExecution() {
    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot current;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId == 0 || state_->state == DebugSessionState::Terminated) {
            return std::unexpected(invalidState("terminate requires an active non-terminated debug session"));
        }
        state_->pauseRequested.store(false, std::memory_order_release);
        state_->terminateRequested.store(true, std::memory_order_release);
        state_->stopReason.reset();
        clearStepLocked(*state_);
        clearExceptionLocked(*state_);
        cancelCommandsLocked(*state_, DebugError{DebugErrorCode::StaleReference,
                                                 "debug inspection cancelled because execution terminated"});
        state_->terminationReason = DebugTerminationReason::Requested;
        state_->state = DebugSessionState::Terminated;
        refreshInstructionFlags(*state_);
        sink = state_->sink.lock();
        current = snapshotLocked(*state_);
    }
    publish(sink, current);
    state_->condition.notify_all();
    return {};
}

SourceId DebugController::registerSourceName(StrView rawSource) {
    return state_->breakpoints.registerSourceName(rawSource);
}

SourceId DebugController::registerFilePath(StrView path) {
    return state_->breakpoints.registerFilePath(path);
}

Opt<RegisteredSource> DebugController::source(SourceId sourceId) const {
    return state_->breakpoints.source(sourceId);
}

DebugResult<Vec<BreakpointBinding>> DebugController::setBreakpoints(SourceId sourceId,
                                                                    std::span<const SourceBreakpoint> requested) {
    DebugResult<Vec<BreakpointBinding>> result = state_->breakpoints.setBreakpoints(sourceId, requested);
    if (result) {
        state_->breakpointEpoch.fetch_add(1, std::memory_order_release);
        std::lock_guard lock(state_->mutex);
        refreshInstructionFlags(*state_);
    }
    return result;
}

DebugResult<Vec<BreakpointBinding>>
DebugController::setFunctionBreakpoints(std::span<const FunctionBreakpoint> requested) {
    DebugResult<Vec<BreakpointBinding>> result = state_->breakpoints.setFunctionBreakpoints(requested);
    if (result) {
        state_->breakpointEpoch.fetch_add(1, std::memory_order_release);
        std::lock_guard lock(state_->mutex);
        refreshInstructionFlags(*state_);
    }
    return result;
}

DebugResult<Vec<DebugThread>> DebugController::threads() {
    std::lock_guard lock(state_->mutex);
    if (state_->activeSessionId == 0) {
        return std::unexpected(invalidState("threads require an active debug session"));
    }
    if (state_->statesByPointer.empty()) {
        const DebugThreadState status = state_->state == DebugSessionState::Suspended ? DebugThreadState::Paused
                                                                                     : DebugThreadState::Running;
        return Vec<DebugThread>{{mainThreadId(), "main", status}};
    }
    Vec<DebugThread> result;
    result.reserve(state_->statesByPointer.size());
    for (const auto& [pointer, registered] : state_->statesByPointer) {
        (void)pointer;
        result.push_back({registered.threadId, registered.name, registered.status});
    }
    std::sort(result.begin(), result.end(),
              [](const DebugThread& left, const DebugThread& right) { return left.id.value() < right.id.value(); });
    return result;
}

DebugResult<Vec<DebugState>> DebugController::states() const {
    std::lock_guard lock(state_->mutex);
    if (state_->activeSessionId == 0) {
        return std::unexpected(invalidState("states require an active debug session"));
    }
    Vec<DebugState> result;
    result.reserve(state_->statesByPointer.size());
    for (const auto& [pointer, registered] : state_->statesByPointer) {
        (void)pointer;
        result.push_back(snapshotStateLocked(*state_, registered));
    }
    std::sort(result.begin(), result.end(),
              [](const DebugState& left, const DebugState& right) { return left.id.value() < right.id.value(); });
    return result;
}

DebugResult<void> DebugController::selectState(StateId stateId) {
    std::lock_guard lock(state_->mutex);
    if (state_->activeSessionId == 0) {
        return std::unexpected(invalidState("state selection requires an active debug session"));
    }
    if (findStateLocked(*state_, stateId) == nullptr) {
        return std::unexpected(DebugError{DebugErrorCode::InvalidReference, "unknown debug state"});
    }
    state_->selectedState = stateId;
    return {};
}

DebugResult<Vec<DebugStackFrame>> DebugController::stackTrace(ThreadId thread, usize startFrame, usize levels) {
    return enqueueOwnerCommand<Vec<DebugStackFrame>>(
        state_, [=](StackInspector& inspector) { return inspector.stackTrace(thread, startFrame, levels); });
}

DebugResult<Vec<DebugScope>> DebugController::scopes(FrameId frame) {
    return enqueueOwnerCommand<Vec<DebugScope>>(state_,
                                                [=](StackInspector& inspector) { return inspector.scopes(frame); });
}

DebugResult<Vec<DebugVariable>> DebugController::variables(VariableReference reference, usize start, usize count,
                                                           DebugVariableFilter filter) {
    return enqueueOwnerCommand<Vec<DebugVariable>>(
        state_, [=](StackInspector& inspector) { return inspector.variables(reference, start, count, filter); });
}

DebugResult<DebugVariable> DebugController::evaluate(FrameId frame, StrView expression) {
    const Str copiedExpression(expression);
    return enqueueOwnerCommand<DebugVariable>(
        state_, [frame, copiedExpression](StackInspector& inspector) {
            return inspector.evaluate(frame, copiedExpression);
        });
}

DebugResult<DebugVariable> DebugController::evaluateWithSideEffects(FrameId frame, StrView expression) {
    {
        std::lock_guard lock(state_->mutex);
        if (!state_->writePolicy.allowSideEffectEvaluation) {
            return std::unexpected(DebugError{DebugErrorCode::PermissionDenied,
                                              "side-effecting evaluation is disabled for this debug session"});
        }
    }
    const Str copiedExpression(expression);
    return enqueueOwnerCommand<DebugVariable>(
        state_, [frame, copiedExpression](StackInspector& inspector) {
            return inspector.evaluateWithSideEffects(frame, copiedExpression);
        });
}

DebugResult<DebugVariable> DebugController::setVariable(VariableReference reference, StrView name,
                                                        StrView valueExpression) {
    {
        std::lock_guard lock(state_->mutex);
        if (!state_->writePolicy.allowVariableWrite) {
            return std::unexpected(DebugError{DebugErrorCode::PermissionDenied,
                                              "variable writes are disabled for this debug session"});
        }
    }
    const Str copiedName(name);
    const Str copiedValue(valueExpression);
    return enqueueOwnerCommand<DebugVariable>(
        state_, [reference, copiedName, copiedValue](StackInspector& inspector) {
            return inspector.setVariable(reference, copiedName, copiedValue);
        });
}

DebugResult<void> DebugController::configureWritePolicy(DebugWritePolicy policy) {
    std::lock_guard lock(state_->mutex);
    if (state_->activeSessionId != 0 && state_->state != DebugSessionState::Starting) {
        return std::unexpected(invalidState("write policy can only change before configurationDone"));
    }
    state_->writePolicy = policy;
    return {};
}

DebugSessionSnapshot DebugController::snapshot() const {
    std::lock_guard lock(state_->mutex);
    return snapshotLocked(*state_);
}

DebugResult<void> DebugController::notifySuspended(DebugStopReason reason) {
    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot current;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId == 0 ||
            (state_->state != DebugSessionState::Running && state_->state != DebugSessionState::PauseRequested)) {
            return std::unexpected(invalidState("a safepoint can suspend only a running or pause-pending session"));
        }
        state_->pauseRequested.store(false, std::memory_order_release);
        state_->pauseGeneration = PauseGeneration{state_->pauseGeneration.value() + 1};
        state_->stopReason = reason;
        state_->state = DebugSessionState::Suspended;
        refreshInstructionFlags(*state_);
        sink = state_->sink.lock();
        current = snapshotLocked(*state_);
    }
    publish(sink, current);
    state_->condition.notify_all();
    return {};
}

DebugResult<void> DebugController::confirmResumed() {
    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot current;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId == 0 || state_->state != DebugSessionState::ResumeRequested) {
            return std::unexpected(invalidState("resume confirmation requires a pending resume"));
        }
        state_->state = DebugSessionState::Running;
        sink = state_->sink.lock();
        current = snapshotLocked(*state_);
    }
    publish(sink, current);
    state_->condition.notify_all();
    return {};
}

void DebugController::notifyProgramTerminated(DebugTerminationReason reason, Opt<DebugError> error) {
    Ptr<IDebugEventSink> sink;
    DebugSessionSnapshot current;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->state == DebugSessionState::Terminated) {
            return;
        }
        state_->pauseRequested.store(false, std::memory_order_release);
        state_->stopReason.reset();
        cancelCommandsLocked(*state_, DebugError{DebugErrorCode::StaleReference,
                                                 "debug inspection cancelled because the program terminated"});
        state_->terminationReason = reason;
        clearStepLocked(*state_);
        clearExceptionLocked(*state_);
        state_->lastError = std::move(error);
        state_->state = DebugSessionState::Terminated;
        refreshInstructionFlags(*state_);
        sink = state_->sink.lock();
        current = snapshotLocked(*state_);
    }
    state_->condition.notify_all();
    publish(sink, current);
}

DebugSafepointResult DebugController::instructionSafepoint() {
    return waitForSafepointRequest(state_);
}

DebugSafepointResult DebugController::instructionSafepoint(LuaState& pausedState, const Proto& proto, usize pc) {
    if (state_->terminateRequested.load(std::memory_order_acquire) ||
        state_->pauseRequested.load(std::memory_order_acquire)) {
        return waitForSafepointRequest(state_, std::nullopt, &pausedState);
    }
    {
        std::lock_guard lock(state_->mutex);
        if (state_->state == DebugSessionState::Running && state_->exceptionPropagationActive) {
            clearExceptionLocked(*state_);
        }
    }
    if (state_->breakpoints.hasBreakpoints()) {
        const u64 epoch = state_->breakpointEpoch.load(std::memory_order_acquire);
        if (state_->ownerBreakpointEpoch != epoch) {
            state_->ownerBreakpointEpoch = epoch;
            state_->suppressingBreakpoint = false;
        }

        const Ptr<const BreakpointHitList> hits = state_->breakpoints.match(proto, pc);
        if (hits == nullptr || hits->empty()) {
            state_->suppressingBreakpoint = false;
        } else if (state_->suppressingBreakpoint && state_->suppressedProto == &proto &&
                   state_->suppressedLine == hits->front().line && pc > state_->suppressedLastPc) {
            state_->suppressedLastPc = pc;
        } else {
            state_->suppressingBreakpoint = true;
            state_->suppressedProto = &proto;
            state_->suppressedLine = hits->front().line;
            state_->suppressedLastPc = pc;
            bool shouldStop = false;
            for (const BreakpointHit& hit : *hits) {
                const Opt<BreakpointActivation> activation = state_->breakpoints.recordHit(hit.id);
                if (!activation || !activation->hitTargetReached) {
                    continue;
                }
                const BreakpointBehavior emptyBehavior;
                const BreakpointBehavior& behavior =
                    activation->behavior == nullptr ? emptyBehavior : *activation->behavior;
                EvaluatedBreakpointBehavior evaluated = evaluateBreakpointBehavior(*state_, pausedState, behavior);
                if (evaluated.error) {
                    const Str prefix = evaluated.conditionError ? "[YanLua breakpoint condition] "
                                                                : "[YanLua log point] ";
                    publishDebugOutput(*state_, prefix + evaluated.error->message + "\n");
                    if (evaluated.conditionError) {
                        shouldStop = true;
                    }
                    continue;
                }
                if (!evaluated.conditionMatched) {
                    continue;
                }
                if (evaluated.logOutput) {
                    publishDebugOutput(*state_, std::move(*evaluated.logOutput));
                    continue;
                }
                shouldStop = true;
            }
            if (shouldStop) {
                return waitForSafepointRequest(state_, DebugStopReason::Breakpoint, &pausedState);
            }
        }
    }

    bool stepStop = false;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId != 0 && state_->state == DebugSessionState::Running) {
            stepStop = shouldStopStepLocked(*state_, pausedState, proto, pc);
        }
    }
    return stepStop ? waitForSafepointRequest(state_, DebugStopReason::Step, &pausedState)
                    : DebugSafepointResult::ContinueExecution;
}

DebugSafepointResult DebugController::semanticSafepoint(DebugSemanticEvent event) {
    Ptr<IDebugEventSink> sink;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId != 0) {
            sink = state_->sink.lock();
        }
    }
    if (sink != nullptr) {
        try {
            sink->onDebugSemanticEvent(event);
        } catch (...) {
            // Debug client failures cannot unwind through VM frame transitions.
        }
    }
    return waitForSafepointRequest(state_);
}

DebugSafepointResult DebugController::semanticSafepoint(LuaState& pausedState, DebugSemanticEvent event) {
    Ptr<IDebugEventSink> sink;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId != 0) {
            sink = state_->sink.lock();
        }
    }
    if (sink != nullptr) {
        try {
            sink->onDebugSemanticEvent(event);
        } catch (...) {
            // Debug client failures cannot unwind through VM frame transitions.
        }
    }
    return waitForSafepointRequest(state_, std::nullopt, &pausedState);
}

DebugSafepointResult DebugController::exceptionSafepoint(LuaState& pausedState, DebugExceptionCategory category,
                                                          StrView description, const Value* errorObject) {
    Ptr<IDebugEventSink> sink;
    bool shouldBreak = false;
    {
        std::lock_guard lock(state_->mutex);
        if (state_->activeSessionId != 0) {
            sink = state_->sink.lock();
        }
        if (state_->activeSessionId != 0 && state_->breakOnAllExceptions &&
            state_->state == DebugSessionState::Running && !state_->exceptionPropagationActive) {
            state_->exceptionPropagationActive = true;
            state_->exception = DebugExceptionInfo{exceptionId(category), Str(description), "always", category};
            if (errorObject != nullptr) {
                state_->exceptionValue = *errorObject;
            } else {
                state_->exceptionValue.reset();
            }
            shouldBreak = true;
        }
    }
    if (sink != nullptr) {
        try {
            sink->onDebugSemanticEvent(DebugSemanticEvent::Exception);
        } catch (...) {
            // Debug client failures cannot unwind through exception propagation.
        }
    }
    return waitForSafepointRequest(state_, shouldBreak ? Opt<DebugStopReason>{DebugStopReason::Exception}
                                                       : Opt<DebugStopReason>{},
                                   &pausedState);
}

void DebugController::registerProto(const Proto& root) {
    const Vec<BreakpointBinding> changed = state_->breakpoints.registerProto(root);
    if (changed.empty()) {
        return;
    }
    state_->breakpointEpoch.fetch_add(1, std::memory_order_release);

    Ptr<IDebugEventSink> sink;
    {
        std::lock_guard lock(state_->mutex);
        refreshInstructionFlags(*state_);
        sink = state_->sink.lock();
    }
    if (sink == nullptr) {
        return;
    }
    for (const BreakpointBinding& breakpoint : changed) {
        try {
            sink->onBreakpointChanged(breakpoint);
        } catch (...) {
            // Breakpoint notifications cannot unwind through Proto registration.
        }
    }
}

StateId DebugController::registerState(LuaState& luaState, StrView name, StrView label) {
    Ptr<IDebugEventSink> sink;
    DebugState published;
    {
        std::lock_guard lock(state_->mutex);
        const auto existing = state_->statesByPointer.find(&luaState);
        if (existing != state_->statesByPointer.end()) {
            return existing->second.id;
        }
        if (state_->statesByPointer.size() >= state_->maxStates || state_->nextStateId == 0 ||
            state_->nextThreadId == 0) {
            return {};
        }

        RegisteredDebugState registered;
        registered.id = StateId{state_->nextStateId++};
        registered.threadId = ThreadId{state_->nextThreadId++};
        registered.state = &luaState;
        registered.name = name.empty() ? (registered.id.value() == 1 ? "main" : "coroutine " +
                                                                           std::to_string(registered.id.value()))
                                       : Str(name);
        registered.label = label.empty() ? registered.name : Str(label);
        const auto [iterator, inserted] = state_->statesByPointer.emplace(&luaState, std::move(registered));
        if (!inserted) {
            return iterator->second.id;
        }
        state_->statesById.emplace(iterator->second.id.value(), &luaState);
        state_->statesByThread.emplace(iterator->second.threadId.value(), &luaState);
        if (!state_->selectedState.valid()) {
            state_->selectedState = iterator->second.id;
        }
        sink = state_->sink.lock();
        published = snapshotStateLocked(*state_, iterator->second);
    }
    if (sink != nullptr) {
        try {
            sink->onDebugExecutionUnitChanged(published, true);
        } catch (...) {
        }
    }
    return published.id;
}

void DebugController::unregisterState(LuaState& luaState) noexcept {
    Ptr<IDebugEventSink> sink;
    Opt<DebugState> published;
    {
        std::lock_guard lock(state_->mutex);
        const auto found = state_->statesByPointer.find(&luaState);
        if (found == state_->statesByPointer.end()) {
            return;
        }
        RegisteredDebugState& registered = found->second;
        registered.status = DebugThreadState::Exited;
        published = snapshotStateLocked(*state_, registered);
        state_->statesById.erase(registered.id.value());
        state_->statesByThread.erase(registered.threadId.value());
        const bool selected = state_->selectedState == registered.id;
        state_->statesByPointer.erase(found);
        if (selected) {
            state_->selectedState = {};
            for (const auto& [pointer, candidate] : state_->statesByPointer) {
                (void)pointer;
                if (!state_->selectedState.valid() || candidate.id.value() < state_->selectedState.value()) {
                    state_->selectedState = candidate.id;
                }
            }
        }
        sink = state_->sink.lock();
    }
    if (sink != nullptr && published) {
        try {
            sink->onDebugExecutionUnitChanged(*published, false);
        } catch (...) {
        }
    }
}

ThreadId DebugController::threadIdForState(const LuaState& luaState) const noexcept {
    std::lock_guard lock(state_->mutex);
    const auto found = state_->statesByPointer.find(&luaState);
    return found == state_->statesByPointer.end() ? ThreadId{} : found->second.threadId;
}

bool DebugController::pauseRequested() const noexcept {
    return state_->pauseRequested.load(std::memory_order_acquire);
}

bool DebugController::terminateRequested() const noexcept {
    return state_->terminateRequested.load(std::memory_order_acquire);
}

void DebugController::shutdown(DisconnectAction action) noexcept {
    u64 sessionId = 0;
    {
        std::lock_guard lock(state_->mutex);
        sessionId = state_->activeSessionId;
    }
    detachSession(state_, sessionId, action);
}

} // namespace Lua::Debugger
