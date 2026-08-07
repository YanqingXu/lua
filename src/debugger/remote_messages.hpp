#pragma once

/**
 * @file remote_messages.hpp
 * @brief Typed YLDP request, response, and event payload codecs.
 */

#include "debugger/debug_runtime.hpp"
#include "debugger/remote_protocol.hpp"

namespace Lua::Debugger::Remote {

struct RemoteBreakpointRequest {
    Str sourcePath;
    Vec<SourceBreakpoint> breakpoints;
};

struct RemoteStackTraceRequest {
    ThreadId thread;
    usize startFrame = 0;
    usize levels = 0;
};

struct RemoteVariablesRequest {
    VariableReference reference;
    usize start = 0;
    usize count = 0;
    DebugVariableFilter filter = DebugVariableFilter::All;
};

struct RemoteEvaluateRequest {
    FrameId frame;
    Str expression;
};

struct RemoteSetVariableRequest {
    VariableReference reference;
    Str name;
    Str valueExpression;
};

struct RemoteStackFrame {
    DebugStackFrame frame;
    Str sourceName;
    bool sourceIsFile = false;
};

struct RemoteStoppedEvent {
    DebugStopReason reason = DebugStopReason::Pause;
    ThreadId thread;
    PauseGeneration generation;
};

struct RemoteTerminatedEvent {
    DebugTerminationReason reason = DebugTerminationReason::Completed;
    Opt<DebugError> error;
};

[[nodiscard]] ProtocolStatus protocolStatus(DebugErrorCode code) noexcept;
[[nodiscard]] DebugErrorCode debugErrorCode(ProtocolStatus status) noexcept;
[[nodiscard]] ProtocolResult<Vec<u8>> encodeErrorMessage(StrView message);
[[nodiscard]] ProtocolResult<Str> decodeErrorMessage(std::span<const u8> payload);

[[nodiscard]] ProtocolResult<Vec<u8>> encodeBreakpointRequest(const RemoteBreakpointRequest& request);
[[nodiscard]] ProtocolResult<RemoteBreakpointRequest> decodeBreakpointRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeAdvancedBreakpointRequest(const RemoteBreakpointRequest& request);
[[nodiscard]] ProtocolResult<RemoteBreakpointRequest> decodeAdvancedBreakpointRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>>
encodeFunctionBreakpointRequest(std::span<const FunctionBreakpoint> breakpoints);
[[nodiscard]] ProtocolResult<Vec<FunctionBreakpoint>>
decodeFunctionBreakpointRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeBreakpointBindings(std::span<const BreakpointBinding> bindings);
[[nodiscard]] ProtocolResult<Vec<BreakpointBinding>> decodeBreakpointBindings(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>>
encodeFunctionBreakpointBindings(std::span<const BreakpointBinding> bindings);
[[nodiscard]] ProtocolResult<Vec<BreakpointBinding>>
decodeFunctionBreakpointBindings(std::span<const u8> payload);

[[nodiscard]] ProtocolResult<Vec<u8>> encodeThreadRequest(ThreadId thread);
[[nodiscard]] ProtocolResult<ThreadId> decodeThreadRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeStackTraceRequest(const RemoteStackTraceRequest& request);
[[nodiscard]] ProtocolResult<RemoteStackTraceRequest> decodeStackTraceRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeFrameRequest(FrameId frame);
[[nodiscard]] ProtocolResult<FrameId> decodeFrameRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeVariablesRequest(const RemoteVariablesRequest& request);
[[nodiscard]] ProtocolResult<RemoteVariablesRequest> decodeVariablesRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeEvaluateRequest(const RemoteEvaluateRequest& request);
[[nodiscard]] ProtocolResult<RemoteEvaluateRequest> decodeEvaluateRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeSetVariableRequest(const RemoteSetVariableRequest& request);
[[nodiscard]] ProtocolResult<RemoteSetVariableRequest> decodeSetVariableRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeBooleanRequest(bool value);
[[nodiscard]] ProtocolResult<bool> decodeBooleanRequest(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeStateRequest(StateId state);
[[nodiscard]] ProtocolResult<StateId> decodeStateRequest(std::span<const u8> payload);

[[nodiscard]] ProtocolResult<Vec<u8>> encodeThreads(std::span<const DebugThread> threads);
[[nodiscard]] ProtocolResult<Vec<DebugThread>> decodeThreads(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeStates(std::span<const DebugState> states);
[[nodiscard]] ProtocolResult<Vec<DebugState>> decodeStates(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeStackFrames(std::span<const RemoteStackFrame> frames);
[[nodiscard]] ProtocolResult<Vec<RemoteStackFrame>> decodeStackFrames(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeScopes(std::span<const DebugScope> scopes);
[[nodiscard]] ProtocolResult<Vec<DebugScope>> decodeScopes(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeVariables(std::span<const DebugVariable> variables);
[[nodiscard]] ProtocolResult<Vec<DebugVariable>> decodeVariables(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeVariable(const DebugVariable& variable);
[[nodiscard]] ProtocolResult<DebugVariable> decodeVariable(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeExceptionInfo(const DebugExceptionInfo& exception);
[[nodiscard]] ProtocolResult<DebugExceptionInfo> decodeExceptionInfo(std::span<const u8> payload);

[[nodiscard]] ProtocolResult<Vec<u8>> encodeStoppedEvent(const RemoteStoppedEvent& event);
[[nodiscard]] ProtocolResult<RemoteStoppedEvent> decodeStoppedEvent(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeTerminatedEvent(const RemoteTerminatedEvent& event);
[[nodiscard]] ProtocolResult<RemoteTerminatedEvent> decodeTerminatedEvent(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeDebugStateEvent(const DebugState& state);
[[nodiscard]] ProtocolResult<DebugState> decodeDebugStateEvent(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeOutputEvent(StrView text, DebugOutputCategory category);
[[nodiscard]] ProtocolResult<std::pair<Str, DebugOutputCategory>> decodeOutputEvent(std::span<const u8> payload);

} // namespace Lua::Debugger::Remote
