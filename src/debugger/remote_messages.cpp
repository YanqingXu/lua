/**
 * @file remote_messages.cpp
 * @brief Typed payload codecs shared by the runtime server and adapter client.
 */

#include "debugger/remote_messages.hpp"

#include <limits>

namespace Lua::Debugger::Remote {

namespace {

ProtocolError malformed(Str message = "malformed YLDP command payload") {
    return {ProtocolStatus::ProtocolError, std::move(message), 0};
}

bool writeCount(ProtocolWriter& writer, usize count) {
    if (count > kProtocolMaxCollectionItems || count > std::numeric_limits<u32>::max()) {
        return false;
    }
    writer.writeU32(static_cast<u32>(count));
    return true;
}

void writeVariable(ProtocolWriter& writer, const DebugVariable& variable) {
    writer.writeString(variable.name);
    writer.writeString(variable.value);
    writer.writeString(variable.type);
    writer.writeBool(variable.evaluateName.has_value());
    if (variable.evaluateName) {
        writer.writeString(*variable.evaluateName);
    }
    writer.writeU64(variable.variablesReference.value());
    writer.writeU64(variable.namedVariables);
    writer.writeU64(variable.indexedVariables);
}

ProtocolResult<DebugVariable> readVariable(ProtocolReader& reader) {
    auto name = reader.readString();
    auto value = reader.readString();
    auto type = reader.readString();
    auto hasEvaluateName = reader.readBool();
    if (!name || !value || !type || !hasEvaluateName) {
        return std::unexpected(malformed());
    }
    Opt<Str> evaluateName;
    if (*hasEvaluateName) {
        auto parsed = reader.readString();
        if (!parsed) {
            return std::unexpected(parsed.error());
        }
        evaluateName = std::move(*parsed);
    }
    auto reference = reader.readU64();
    auto named = reader.readU64();
    auto indexed = reader.readU64();
    if (!reference || !named || !indexed || *named > std::numeric_limits<usize>::max() ||
        *indexed > std::numeric_limits<usize>::max()) {
        return std::unexpected(malformed());
    }
    return DebugVariable{std::move(*name),
                         std::move(*value),
                         std::move(*type),
                         std::move(evaluateName),
                         VariableReference{*reference},
                         static_cast<usize>(*named),
                         static_cast<usize>(*indexed)};
}

} // namespace

ProtocolStatus protocolStatus(DebugErrorCode code) noexcept {
    switch (code) {
    case DebugErrorCode::InvalidState:
        return ProtocolStatus::InvalidState;
    case DebugErrorCode::InvalidReference:
        return ProtocolStatus::NotFound;
    case DebugErrorCode::StaleReference:
        return ProtocolStatus::StaleReference;
    case DebugErrorCode::ResourceLimit:
        return ProtocolStatus::ResourceLimit;
    case DebugErrorCode::Timeout:
        return ProtocolStatus::Timeout;
    case DebugErrorCode::Unsupported:
        return ProtocolStatus::NotSupported;
    case DebugErrorCode::RuntimeFailure:
        return ProtocolStatus::InternalError;
    }
    return ProtocolStatus::InternalError;
}

DebugErrorCode debugErrorCode(ProtocolStatus status) noexcept {
    switch (status) {
    case ProtocolStatus::InvalidState:
        return DebugErrorCode::InvalidState;
    case ProtocolStatus::StaleReference:
        return DebugErrorCode::StaleReference;
    case ProtocolStatus::ResourceLimit:
        return DebugErrorCode::ResourceLimit;
    case ProtocolStatus::Timeout:
        return DebugErrorCode::Timeout;
    case ProtocolStatus::NotSupported:
        return DebugErrorCode::Unsupported;
    case ProtocolStatus::InvalidArgument:
    case ProtocolStatus::NotFound:
        return DebugErrorCode::InvalidReference;
    default:
        return DebugErrorCode::RuntimeFailure;
    }
}

ProtocolResult<Vec<u8>> encodeErrorMessage(StrView message) {
    ProtocolWriter writer;
    writer.writeString(message);
    return std::move(writer).finish();
}

ProtocolResult<Str> decodeErrorMessage(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto message = reader.readString();
    if (!message) {
        return std::unexpected(message.error());
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return message;
}

ProtocolResult<Vec<u8>> encodeBreakpointRequest(const RemoteBreakpointRequest& request) {
    ProtocolWriter writer;
    writer.writeString(request.sourcePath);
    if (!writeCount(writer, request.breakpoints.size())) {
        return std::unexpected(ProtocolError{ProtocolStatus::ResourceLimit, "too many remote breakpoints", 0});
    }
    for (const SourceBreakpoint& breakpoint : request.breakpoints) {
        writer.writeI64(breakpoint.line);
    }
    return std::move(writer).finish();
}

ProtocolResult<RemoteBreakpointRequest> decodeBreakpointRequest(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto source = reader.readString();
    auto count = reader.readCount();
    if (!source || !count || source->empty()) {
        return std::unexpected(malformed("remote breakpoint source must be non-empty"));
    }
    RemoteBreakpointRequest result;
    result.sourcePath = std::move(*source);
    result.breakpoints.reserve(*count);
    for (usize index = 0; index < *count; ++index) {
        auto line = reader.readI64();
        if (!line || *line < 1 || *line > std::numeric_limits<i32>::max()) {
            return std::unexpected(malformed("remote breakpoint line is out of range"));
        }
        result.breakpoints.push_back({static_cast<i32>(*line)});
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return result;
}

ProtocolResult<Vec<u8>> encodeBreakpointBindings(std::span<const BreakpointBinding> bindings) {
    ProtocolWriter writer;
    if (!writeCount(writer, bindings.size())) {
        return std::unexpected(ProtocolError{ProtocolStatus::ResourceLimit, "too many breakpoint bindings", 0});
    }
    for (const BreakpointBinding& binding : bindings) {
        writer.writeU64(binding.id.value());
        writer.writeU64(binding.sourceId.value());
        writer.writeI64(binding.requestedLine);
        writer.writeI64(binding.line);
        writer.writeBool(binding.verified);
        writer.writeString(binding.message);
    }
    return std::move(writer).finish();
}

ProtocolResult<Vec<BreakpointBinding>> decodeBreakpointBindings(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto count = reader.readCount();
    if (!count) {
        return std::unexpected(count.error());
    }
    Vec<BreakpointBinding> result;
    result.reserve(*count);
    for (usize index = 0; index < *count; ++index) {
        auto id = reader.readU64();
        auto source = reader.readU64();
        auto requested = reader.readI64();
        auto line = reader.readI64();
        auto verified = reader.readBool();
        auto message = reader.readString();
        if (!id || !source || !requested || !line || !verified || !message ||
            *requested < std::numeric_limits<i32>::min() || *requested > std::numeric_limits<i32>::max() ||
            *line < std::numeric_limits<i32>::min() || *line > std::numeric_limits<i32>::max()) {
            return std::unexpected(malformed());
        }
        result.push_back({BreakpointId{*id}, SourceId{*source}, static_cast<i32>(*requested), static_cast<i32>(*line),
                          *verified, std::move(*message)});
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return result;
}

ProtocolResult<Vec<u8>> encodeThreadRequest(ThreadId thread) {
    ProtocolWriter writer;
    writer.writeU64(thread.value());
    return std::move(writer).finish();
}

ProtocolResult<ThreadId> decodeThreadRequest(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto id = reader.readU64();
    if (!id || *id == 0) {
        return std::unexpected(malformed("remote thread ID must be non-zero"));
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return ThreadId{*id};
}

ProtocolResult<Vec<u8>> encodeStackTraceRequest(const RemoteStackTraceRequest& request) {
    ProtocolWriter writer;
    writer.writeU64(request.thread.value());
    writer.writeU64(request.startFrame);
    writer.writeU64(request.levels);
    return std::move(writer).finish();
}

ProtocolResult<RemoteStackTraceRequest> decodeStackTraceRequest(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto thread = reader.readU64();
    auto start = reader.readU64();
    auto levels = reader.readU64();
    if (!thread || !start || !levels || *thread == 0 || *start > std::numeric_limits<usize>::max() ||
        *levels > std::numeric_limits<usize>::max()) {
        return std::unexpected(malformed());
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return RemoteStackTraceRequest{ThreadId{*thread}, static_cast<usize>(*start), static_cast<usize>(*levels)};
}

ProtocolResult<Vec<u8>> encodeFrameRequest(FrameId frame) {
    ProtocolWriter writer;
    writer.writeU64(frame.value());
    return std::move(writer).finish();
}

ProtocolResult<FrameId> decodeFrameRequest(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto id = reader.readU64();
    if (!id || *id == 0) {
        return std::unexpected(malformed("remote frame ID must be non-zero"));
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return FrameId{*id};
}

ProtocolResult<Vec<u8>> encodeVariablesRequest(const RemoteVariablesRequest& request) {
    ProtocolWriter writer;
    writer.writeU64(request.reference.value());
    writer.writeU64(request.start);
    writer.writeU64(request.count);
    writer.writeU8(static_cast<u8>(request.filter));
    return std::move(writer).finish();
}

ProtocolResult<RemoteVariablesRequest> decodeVariablesRequest(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto reference = reader.readU64();
    auto start = reader.readU64();
    auto count = reader.readU64();
    auto filter = reader.readU8();
    if (!reference || !start || !count || !filter || *reference == 0 || *start > std::numeric_limits<usize>::max() ||
        *count > std::numeric_limits<usize>::max() || *filter > static_cast<u8>(DebugVariableFilter::Named)) {
        return std::unexpected(malformed());
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return RemoteVariablesRequest{VariableReference{*reference}, static_cast<usize>(*start),
                                  static_cast<usize>(*count), static_cast<DebugVariableFilter>(*filter)};
}

ProtocolResult<Vec<u8>> encodeEvaluateRequest(const RemoteEvaluateRequest& request) {
    ProtocolWriter writer;
    writer.writeU64(request.frame.value());
    writer.writeString(request.expression);
    return std::move(writer).finish();
}

ProtocolResult<RemoteEvaluateRequest> decodeEvaluateRequest(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto frame = reader.readU64();
    auto expression = reader.readString();
    if (!frame || !expression || *frame == 0 || expression->empty()) {
        return std::unexpected(malformed());
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return RemoteEvaluateRequest{FrameId{*frame}, std::move(*expression)};
}

ProtocolResult<Vec<u8>> encodeBooleanRequest(bool value) {
    ProtocolWriter writer;
    writer.writeBool(value);
    return std::move(writer).finish();
}

ProtocolResult<bool> decodeBooleanRequest(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto value = reader.readBool();
    if (!value) {
        return std::unexpected(value.error());
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return value;
}

ProtocolResult<Vec<u8>> encodeStateRequest(StateId state) {
    ProtocolWriter writer;
    writer.writeU64(state.value());
    return std::move(writer).finish();
}

ProtocolResult<StateId> decodeStateRequest(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto id = reader.readU64();
    if (!id || *id == 0) {
        return std::unexpected(malformed("remote state ID must be non-zero"));
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return StateId{*id};
}

ProtocolResult<Vec<u8>> encodeThreads(std::span<const DebugThread> threads) {
    ProtocolWriter writer;
    if (!writeCount(writer, threads.size())) {
        return std::unexpected(ProtocolError{ProtocolStatus::ResourceLimit, "too many debug threads", 0});
    }
    for (const DebugThread& thread : threads) {
        writer.writeU64(thread.id.value());
        writer.writeString(thread.name);
        writer.writeU8(static_cast<u8>(thread.state));
    }
    return std::move(writer).finish();
}

ProtocolResult<Vec<DebugThread>> decodeThreads(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto count = reader.readCount();
    if (!count) {
        return std::unexpected(count.error());
    }
    Vec<DebugThread> result;
    result.reserve(*count);
    for (usize index = 0; index < *count; ++index) {
        auto id = reader.readU64();
        auto name = reader.readString();
        auto state = reader.readU8();
        if (!id || !name || !state || *id == 0 || *state > static_cast<u8>(DebugThreadState::Exited)) {
            return std::unexpected(malformed());
        }
        result.push_back({ThreadId{*id}, std::move(*name), static_cast<DebugThreadState>(*state)});
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return result;
}

ProtocolResult<Vec<u8>> encodeStates(std::span<const DebugState> states) {
    ProtocolWriter writer;
    if (!writeCount(writer, states.size())) {
        return std::unexpected(ProtocolError{ProtocolStatus::ResourceLimit, "too many debug states", 0});
    }
    for (const DebugState& state : states) {
        writer.writeU64(state.id.value());
        writer.writeU64(state.threadId.value());
        writer.writeString(state.name);
        writer.writeString(state.label);
        writer.writeU8(static_cast<u8>(state.state));
        writer.writeBool(state.selected);
    }
    return std::move(writer).finish();
}

ProtocolResult<Vec<DebugState>> decodeStates(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto count = reader.readCount();
    if (!count) {
        return std::unexpected(count.error());
    }
    Vec<DebugState> result;
    result.reserve(*count);
    for (usize index = 0; index < *count; ++index) {
        auto id = reader.readU64();
        auto thread = reader.readU64();
        auto name = reader.readString();
        auto label = reader.readString();
        auto state = reader.readU8();
        auto selected = reader.readBool();
        if (!id || !thread || !name || !label || !state || !selected || *id == 0 || *thread == 0 ||
            *state > static_cast<u8>(DebugThreadState::Exited)) {
            return std::unexpected(malformed());
        }
        result.push_back({StateId{*id}, ThreadId{*thread}, std::move(*name), std::move(*label),
                          static_cast<DebugThreadState>(*state), *selected});
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return result;
}

ProtocolResult<Vec<u8>> encodeStackFrames(std::span<const RemoteStackFrame> frames) {
    ProtocolWriter writer;
    if (!writeCount(writer, frames.size())) {
        return std::unexpected(ProtocolError{ProtocolStatus::ResourceLimit, "too many stack frames", 0});
    }
    for (const RemoteStackFrame& remote : frames) {
        const DebugStackFrame& frame = remote.frame;
        writer.writeU64(frame.id.value());
        writer.writeU64(frame.threadId.value());
        writer.writeString(frame.name);
        writer.writeU64(frame.location.sourceId.value());
        writer.writeI64(frame.location.line);
        writer.writeI64(frame.location.column);
        writer.writeU64(frame.location.pc);
        writer.writeBool(frame.native);
        writer.writeString(remote.sourceName);
        writer.writeBool(remote.sourceIsFile);
    }
    return std::move(writer).finish();
}

ProtocolResult<Vec<RemoteStackFrame>> decodeStackFrames(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto count = reader.readCount();
    if (!count) {
        return std::unexpected(count.error());
    }
    Vec<RemoteStackFrame> result;
    result.reserve(*count);
    for (usize index = 0; index < *count; ++index) {
        auto id = reader.readU64();
        auto thread = reader.readU64();
        auto name = reader.readString();
        auto source = reader.readU64();
        auto line = reader.readI64();
        auto column = reader.readI64();
        auto pc = reader.readU64();
        auto native = reader.readBool();
        auto sourceName = reader.readString();
        auto sourceIsFile = reader.readBool();
        if (!id || !thread || !name || !source || !line || !column || !pc || !native || !sourceName ||
            !sourceIsFile || *line < std::numeric_limits<i32>::min() || *line > std::numeric_limits<i32>::max() ||
            *column < std::numeric_limits<i32>::min() || *column > std::numeric_limits<i32>::max() ||
            *pc > std::numeric_limits<usize>::max()) {
            return std::unexpected(malformed());
        }
        DebugStackFrame frame{FrameId{*id},
                              ThreadId{*thread},
                              std::move(*name),
                              SourceLocation{SourceId{*source}, static_cast<i32>(*line), static_cast<i32>(*column),
                                             static_cast<usize>(*pc)},
                              *native};
        result.push_back({std::move(frame), std::move(*sourceName), *sourceIsFile});
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return result;
}

ProtocolResult<Vec<u8>> encodeScopes(std::span<const DebugScope> scopes) {
    ProtocolWriter writer;
    if (!writeCount(writer, scopes.size())) {
        return std::unexpected(ProtocolError{ProtocolStatus::ResourceLimit, "too many scopes", 0});
    }
    for (const DebugScope& scope : scopes) {
        writer.writeString(scope.name);
        writer.writeU8(static_cast<u8>(scope.kind));
        writer.writeU64(scope.variablesReference.value());
        writer.writeBool(scope.expensive);
    }
    return std::move(writer).finish();
}

ProtocolResult<Vec<DebugScope>> decodeScopes(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto count = reader.readCount();
    if (!count) {
        return std::unexpected(count.error());
    }
    Vec<DebugScope> result;
    result.reserve(*count);
    for (usize index = 0; index < *count; ++index) {
        auto name = reader.readString();
        auto kind = reader.readU8();
        auto reference = reader.readU64();
        auto expensive = reader.readBool();
        if (!name || !kind || !reference || !expensive || *kind > static_cast<u8>(DebugScopeKind::Exception)) {
            return std::unexpected(malformed());
        }
        result.push_back({std::move(*name), static_cast<DebugScopeKind>(*kind), VariableReference{*reference},
                          *expensive});
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return result;
}

ProtocolResult<Vec<u8>> encodeVariables(std::span<const DebugVariable> variables) {
    ProtocolWriter writer;
    if (!writeCount(writer, variables.size())) {
        return std::unexpected(ProtocolError{ProtocolStatus::ResourceLimit, "too many variables", 0});
    }
    for (const DebugVariable& variable : variables) {
        writeVariable(writer, variable);
    }
    return std::move(writer).finish();
}

ProtocolResult<Vec<DebugVariable>> decodeVariables(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto count = reader.readCount();
    if (!count) {
        return std::unexpected(count.error());
    }
    Vec<DebugVariable> result;
    result.reserve(*count);
    for (usize index = 0; index < *count; ++index) {
        auto variable = readVariable(reader);
        if (!variable) {
            return std::unexpected(variable.error());
        }
        result.push_back(std::move(*variable));
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return result;
}

ProtocolResult<Vec<u8>> encodeVariable(const DebugVariable& variable) {
    ProtocolWriter writer;
    writeVariable(writer, variable);
    return std::move(writer).finish();
}

ProtocolResult<DebugVariable> decodeVariable(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto variable = readVariable(reader);
    if (!variable) {
        return std::unexpected(variable.error());
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return variable;
}

ProtocolResult<Vec<u8>> encodeExceptionInfo(const DebugExceptionInfo& exception) {
    ProtocolWriter writer;
    writer.writeString(exception.exceptionId);
    writer.writeString(exception.description);
    writer.writeString(exception.breakMode);
    writer.writeU8(static_cast<u8>(exception.category));
    return std::move(writer).finish();
}

ProtocolResult<DebugExceptionInfo> decodeExceptionInfo(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto id = reader.readString();
    auto description = reader.readString();
    auto breakMode = reader.readString();
    auto category = reader.readU8();
    if (!id || !description || !breakMode || !category ||
        *category > static_cast<u8>(DebugExceptionCategory::HostCancellation)) {
        return std::unexpected(malformed());
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return DebugExceptionInfo{std::move(*id), std::move(*description), std::move(*breakMode),
                              static_cast<DebugExceptionCategory>(*category)};
}

ProtocolResult<Vec<u8>> encodeStoppedEvent(const RemoteStoppedEvent& event) {
    ProtocolWriter writer;
    writer.writeU8(static_cast<u8>(event.reason));
    writer.writeU64(event.thread.value());
    writer.writeU64(event.generation.value());
    return std::move(writer).finish();
}

ProtocolResult<RemoteStoppedEvent> decodeStoppedEvent(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto reason = reader.readU8();
    auto thread = reader.readU64();
    auto generation = reader.readU64();
    if (!reason || !thread || !generation || *reason > static_cast<u8>(DebugStopReason::Exception) || *thread == 0 ||
        *generation == 0) {
        return std::unexpected(malformed());
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return RemoteStoppedEvent{static_cast<DebugStopReason>(*reason), ThreadId{*thread}, PauseGeneration{*generation}};
}

ProtocolResult<Vec<u8>> encodeTerminatedEvent(const RemoteTerminatedEvent& event) {
    ProtocolWriter writer;
    writer.writeU8(static_cast<u8>(event.reason));
    writer.writeBool(event.error.has_value());
    if (event.error) {
        writer.writeU8(static_cast<u8>(event.error->code));
        writer.writeString(event.error->message);
        writer.writeBool(event.error->retryable);
    }
    return std::move(writer).finish();
}

ProtocolResult<RemoteTerminatedEvent> decodeTerminatedEvent(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    auto reason = reader.readU8();
    auto hasError = reader.readBool();
    if (!reason || !hasError || *reason > static_cast<u8>(DebugTerminationReason::Cancelled)) {
        return std::unexpected(malformed());
    }
    Opt<DebugError> error;
    if (*hasError) {
        auto code = reader.readU8();
        auto message = reader.readString();
        auto retryable = reader.readBool();
        if (!code || !message || !retryable || *code > static_cast<u8>(DebugErrorCode::RuntimeFailure)) {
            return std::unexpected(malformed());
        }
        error = DebugError{static_cast<DebugErrorCode>(*code), std::move(*message), *retryable};
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    return RemoteTerminatedEvent{static_cast<DebugTerminationReason>(*reason), std::move(error)};
}

ProtocolResult<Vec<u8>> encodeDebugStateEvent(const DebugState& state) {
    return encodeStates(std::span<const DebugState>(&state, 1));
}

ProtocolResult<DebugState> decodeDebugStateEvent(std::span<const u8> payload) {
    auto states = decodeStates(payload);
    if (!states || states->size() != 1) {
        return std::unexpected(states ? malformed("state event must contain exactly one state") : states.error());
    }
    return std::move(states->front());
}

} // namespace Lua::Debugger::Remote
