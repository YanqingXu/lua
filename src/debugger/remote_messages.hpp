#pragma once

/**
 * @file remote_messages.hpp
 * @brief Typed YLDP request, response, and event payload codecs.
 */

#include "debugger/debug_runtime.hpp"
#include "common/types.hpp"
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
[[nodiscard]] ProtocolResult<Str> decodeErrorMessage(Span<const u8> payload);

[[nodiscard]] ProtocolResult<Vec<u8>> encodeBreakpointRequest(const RemoteBreakpointRequest& request);
[[nodiscard]] ProtocolResult<RemoteBreakpointRequest> decodeBreakpointRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeAdvancedBreakpointRequest(const RemoteBreakpointRequest& request);
[[nodiscard]] ProtocolResult<RemoteBreakpointRequest> decodeAdvancedBreakpointRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeFunctionBreakpointRequest(Span<const FunctionBreakpoint> breakpoints);
[[nodiscard]] ProtocolResult<Vec<FunctionBreakpoint>> decodeFunctionBreakpointRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeBreakpointBindings(Span<const BreakpointBinding> bindings);
[[nodiscard]] ProtocolResult<Vec<BreakpointBinding>> decodeBreakpointBindings(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeFunctionBreakpointBindings(Span<const BreakpointBinding> bindings);
[[nodiscard]] ProtocolResult<Vec<BreakpointBinding>> decodeFunctionBreakpointBindings(Span<const u8> payload);

[[nodiscard]] ProtocolResult<Vec<u8>> encodeThreadRequest(ThreadId thread);
[[nodiscard]] ProtocolResult<ThreadId> decodeThreadRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeStackTraceRequest(const RemoteStackTraceRequest& request);
[[nodiscard]] ProtocolResult<RemoteStackTraceRequest> decodeStackTraceRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeFrameRequest(FrameId frame);
[[nodiscard]] ProtocolResult<FrameId> decodeFrameRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeVariablesRequest(const RemoteVariablesRequest& request);
[[nodiscard]] ProtocolResult<RemoteVariablesRequest> decodeVariablesRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeEvaluateRequest(const RemoteEvaluateRequest& request);
[[nodiscard]] ProtocolResult<RemoteEvaluateRequest> decodeEvaluateRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeSetVariableRequest(const RemoteSetVariableRequest& request);
[[nodiscard]] ProtocolResult<RemoteSetVariableRequest> decodeSetVariableRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeBooleanRequest(bool value);
[[nodiscard]] ProtocolResult<bool> decodeBooleanRequest(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeStateRequest(StateId state);
[[nodiscard]] ProtocolResult<StateId> decodeStateRequest(Span<const u8> payload);

[[nodiscard]] ProtocolResult<Vec<u8>> encodeThreads(Span<const DebugThread> threads);
[[nodiscard]] ProtocolResult<Vec<DebugThread>> decodeThreads(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeStates(Span<const DebugState> states);
[[nodiscard]] ProtocolResult<Vec<DebugState>> decodeStates(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeStackFrames(Span<const RemoteStackFrame> frames);
[[nodiscard]] ProtocolResult<Vec<RemoteStackFrame>> decodeStackFrames(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeScopes(Span<const DebugScope> scopes);
[[nodiscard]] ProtocolResult<Vec<DebugScope>> decodeScopes(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeVariables(Span<const DebugVariable> variables);
[[nodiscard]] ProtocolResult<Vec<DebugVariable>> decodeVariables(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeVariable(const DebugVariable& variable);
[[nodiscard]] ProtocolResult<DebugVariable> decodeVariable(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeExceptionInfo(const DebugExceptionInfo& exception);
[[nodiscard]] ProtocolResult<DebugExceptionInfo> decodeExceptionInfo(Span<const u8> payload);

[[nodiscard]] ProtocolResult<Vec<u8>> encodeStoppedEvent(const RemoteStoppedEvent& event);
[[nodiscard]] ProtocolResult<RemoteStoppedEvent> decodeStoppedEvent(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeTerminatedEvent(const RemoteTerminatedEvent& event);
[[nodiscard]] ProtocolResult<RemoteTerminatedEvent> decodeTerminatedEvent(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeDebugStateEvent(const DebugState& state);
[[nodiscard]] ProtocolResult<DebugState> decodeDebugStateEvent(Span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeOutputEvent(StrView text, DebugOutputCategory category);
[[nodiscard]] ProtocolResult<std::pair<Str, DebugOutputCategory>> decodeOutputEvent(Span<const u8> payload);

} // namespace Lua::Debugger::Remote
