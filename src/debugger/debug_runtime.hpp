#pragma once

/**
 * @file debug_runtime.hpp
 * @brief Editor-independent debugger session and lifecycle interface.
 */

#include "debugger/breakpoint_manager.hpp"

#include <atomic>

namespace Lua::Debugger {

enum class DebugSessionState : u8 {
    Detached,
    Starting,
    Running,
    PauseRequested,
    Suspended,
    ResumeRequested,
    Terminated,
};

enum class DebugStopReason : u8 {
    Entry,
    Breakpoint,
    Pause,
    Step,
    Exception,
};

enum class DebugTerminationReason : u8 {
    Completed,
    Requested,
    RuntimeError,
    Cancelled,
};

enum class DisconnectAction : u8 {
    ContinueExecution,
    TerminateExecution,
};

enum class DebugSemanticEvent : u8 {
    FunctionEnter,
    FunctionReturn,
    Exception,
    CoroutineYield,
    CoroutineResume,
};

enum class DebugSafepointResult : u8 {
    ContinueExecution,
    TerminateExecution,
};

struct DebugSessionSnapshot {
    DebugSessionState state = DebugSessionState::Detached;
    Opt<DebugStopReason> stopReason;
    Opt<DebugTerminationReason> terminationReason;
    Opt<DebugError> lastError;
    PauseGeneration pauseGeneration;
    bool attached = false;
    bool pausePending = false;
    bool terminatePending = false;
    ThreadId activeThread{1};
};

class IDebugEventSink {
public:
    virtual ~IDebugEventSink() = default;
    virtual void onDebugStateChanged(const DebugSessionSnapshot& snapshot) = 0;
    virtual void onDebugSemanticEvent(DebugSemanticEvent) {}
    virtual void onBreakpointChanged(const BreakpointBinding&) {}
    virtual void onDebugExecutionUnitChanged(const DebugState&, bool) {}
};

struct DebugControllerState;

/** A move-only RAII attachment. Destruction applies the configured policy. */
class DebugSession {
public:
    DebugSession() = default;
    ~DebugSession();

    DebugSession(const DebugSession&) = delete;
    DebugSession& operator=(const DebugSession&) = delete;
    DebugSession(DebugSession&& other) noexcept;
    DebugSession& operator=(DebugSession&& other) noexcept;

    [[nodiscard]] bool attached() const noexcept;
    void disconnect(DisconnectAction action) noexcept;

    /** Create a lease for an IDebugRuntime proxy that owns no DebugControllerState. */
    [[nodiscard]] static DebugSession proxy(Func<bool()> attached, Func<void(DisconnectAction)> disconnect,
                                            DisconnectAction disconnectAction = DisconnectAction::ContinueExecution);

private:
    friend class DebugController;
    DebugSession(WPtr<DebugControllerState> state, u64 sessionId, DisconnectAction disconnectAction) noexcept;

    WPtr<DebugControllerState> state_;
    u64 sessionId_ = 0;
    DisconnectAction disconnectAction_ = DisconnectAction::ContinueExecution;
    Func<bool()> proxyAttached_;
    Func<void(DisconnectAction)> proxyDisconnect_;
};

class IDebugRuntime {
public:
    virtual ~IDebugRuntime() = default;

    [[nodiscard]] virtual DebugResult<DebugSession>
    attachSession(Ptr<IDebugEventSink> sink = {},
                  DisconnectAction disconnectAction = DisconnectAction::ContinueExecution) = 0;
    [[nodiscard]] virtual DebugResult<void> configurationDone() = 0;
    [[nodiscard]] virtual DebugResult<void> pause(ThreadId thread) = 0;
    [[nodiscard]] virtual DebugResult<void> continueExecution(ThreadId thread) = 0;
    [[nodiscard]] virtual DebugResult<void> stepExecution(ThreadId thread, DebugStepMode mode) = 0;
    [[nodiscard]] virtual DebugResult<void> setExceptionBreakpoints(bool breakOnAll) = 0;
    [[nodiscard]] virtual DebugResult<DebugExceptionInfo> exceptionInfo(ThreadId thread) = 0;
    [[nodiscard]] virtual DebugResult<void> terminateExecution() = 0;
    [[nodiscard]] virtual SourceId registerSourceName(StrView rawSource) = 0;
    [[nodiscard]] virtual SourceId registerFilePath(StrView path) = 0;
    [[nodiscard]] virtual Opt<RegisteredSource> source(SourceId sourceId) const = 0;
    [[nodiscard]] virtual DebugResult<Vec<BreakpointBinding>>
    setBreakpoints(SourceId sourceId, std::span<const SourceBreakpoint> requested) = 0;
    [[nodiscard]] virtual DebugResult<Vec<DebugThread>> threads() = 0;
    [[nodiscard]] virtual DebugResult<Vec<DebugState>> states() const = 0;
    [[nodiscard]] virtual DebugResult<void> selectState(StateId state) = 0;
    [[nodiscard]] virtual DebugResult<Vec<DebugStackFrame>> stackTrace(ThreadId thread, usize startFrame,
                                                                       usize levels) = 0;
    [[nodiscard]] virtual DebugResult<Vec<DebugScope>> scopes(FrameId frame) = 0;
    [[nodiscard]] virtual DebugResult<Vec<DebugVariable>>
    variables(VariableReference reference, usize start, usize count,
              DebugVariableFilter filter = DebugVariableFilter::All) = 0;
    [[nodiscard]] virtual DebugResult<DebugVariable> evaluate(FrameId frame, StrView expression) = 0;
    [[nodiscard]] virtual DebugSessionSnapshot snapshot() const = 0;
};

/**
 * Runtime-owned session state. VM object inspection remains on the owner thread;
 * cross-thread callers only modify atomic requests and copied lifecycle state.
 */
class DebugController final : public IDebugRuntime {
public:
    explicit DebugController(DebugResourceLimits limits = {});
    ~DebugController() override;

    DebugController(const DebugController&) = delete;
    DebugController& operator=(const DebugController&) = delete;

    [[nodiscard]] DebugResult<DebugSession>
    attachSession(Ptr<IDebugEventSink> sink = {},
                  DisconnectAction disconnectAction = DisconnectAction::ContinueExecution) override;
    [[nodiscard]] DebugResult<void> configurationDone() override;
    [[nodiscard]] DebugResult<void> pause(ThreadId thread) override;
    [[nodiscard]] DebugResult<void> continueExecution(ThreadId thread) override;
    [[nodiscard]] DebugResult<void> stepExecution(ThreadId thread, DebugStepMode mode) override;
    [[nodiscard]] DebugResult<void> setExceptionBreakpoints(bool breakOnAll) override;
    [[nodiscard]] DebugResult<DebugExceptionInfo> exceptionInfo(ThreadId thread) override;
    [[nodiscard]] DebugResult<void> terminateExecution() override;
    [[nodiscard]] SourceId registerSourceName(StrView rawSource) override;
    [[nodiscard]] SourceId registerFilePath(StrView path) override;
    [[nodiscard]] Opt<RegisteredSource> source(SourceId sourceId) const override;
    [[nodiscard]] DebugResult<Vec<BreakpointBinding>>
    setBreakpoints(SourceId sourceId, std::span<const SourceBreakpoint> requested) override;
    [[nodiscard]] DebugResult<Vec<DebugThread>> threads() override;
    [[nodiscard]] DebugResult<Vec<DebugState>> states() const override;
    [[nodiscard]] DebugResult<void> selectState(StateId state) override;
    [[nodiscard]] DebugResult<Vec<DebugStackFrame>> stackTrace(ThreadId thread, usize startFrame,
                                                               usize levels) override;
    [[nodiscard]] DebugResult<Vec<DebugScope>> scopes(FrameId frame) override;
    [[nodiscard]] DebugResult<Vec<DebugVariable>>
    variables(VariableReference reference, usize start, usize count,
              DebugVariableFilter filter = DebugVariableFilter::All) override;
    [[nodiscard]] DebugResult<DebugVariable> evaluate(FrameId frame, StrView expression) override;
    [[nodiscard]] DebugSessionSnapshot snapshot() const override;

    /** VM-owner-thread lifecycle notifications. */
    [[nodiscard]] DebugResult<void> notifySuspended(DebugStopReason reason);
    [[nodiscard]] DebugResult<void> confirmResumed();
    void notifyProgramTerminated(DebugTerminationReason reason, Opt<DebugError> error = {});

    /** Called only by the VM owner thread at safe frame/bytecode boundaries. */
    [[nodiscard]] DebugSafepointResult instructionSafepoint();
    [[nodiscard]] DebugSafepointResult instructionSafepoint(LuaState& state, const Proto& proto, usize pc);
    [[nodiscard]] DebugSafepointResult semanticSafepoint(DebugSemanticEvent event);
    [[nodiscard]] DebugSafepointResult semanticSafepoint(LuaState& state, DebugSemanticEvent event);
    [[nodiscard]] DebugSafepointResult exceptionSafepoint(LuaState& state, DebugExceptionCategory category,
                                                          StrView description, const Value* errorObject = nullptr);
    void registerProto(const Proto& root);

    /** Owner-thread registration for root states and coroutines. */
    [[nodiscard]] StateId registerState(LuaState& state, StrView name = {}, StrView label = {});
    void unregisterState(LuaState& state) noexcept;
    [[nodiscard]] ThreadId threadIdForState(const LuaState& state) const noexcept;

    /** Low-cost request probes used by future VM safepoints. */
    [[nodiscard]] bool pauseRequested() const noexcept;
    [[nodiscard]] bool terminateRequested() const noexcept;

    [[nodiscard]] bool requiresInstructionSafepoint() const noexcept {
        return instructionFlags_.load(std::memory_order_relaxed) != 0;
    }

    /** Idempotently disconnect an attached client before destroying the runtime. */
    void shutdown(DisconnectAction action) noexcept;

    [[nodiscard]] static constexpr ThreadId mainThreadId() noexcept {
        return ThreadId{1};
    }

private:
    std::atomic<u8> instructionFlags_ = 0;
    Ptr<DebugControllerState> state_;
};

} // namespace Lua::Debugger
