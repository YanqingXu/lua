#pragma once

/**
 * @file debug_types.hpp
 * @brief Protocol-independent debugger value types and opaque identifiers.
 */

#include "common/types.hpp"

#include <compare>
#include <expected>

namespace Lua::Debugger {

template <typename Tag> class DebugId {
public:
    constexpr DebugId() noexcept = default;
    constexpr explicit DebugId(u64 value) noexcept : value_(value) {}

    [[nodiscard]] constexpr u64 value() const noexcept {
        return value_;
    }

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value_ != 0;
    }

    constexpr auto operator<=>(const DebugId&) const noexcept = default;

private:
    u64 value_ = 0;
};

struct SourceIdTag;
struct StateIdTag;
struct ThreadIdTag;
struct FrameIdTag;
struct VariableReferenceTag;
struct PauseGenerationTag;
struct BreakpointIdTag;

using SourceId = DebugId<SourceIdTag>;
using StateId = DebugId<StateIdTag>;
using ThreadId = DebugId<ThreadIdTag>;
using FrameId = DebugId<FrameIdTag>;
using VariableReference = DebugId<VariableReferenceTag>;
using PauseGeneration = DebugId<PauseGenerationTag>;
using BreakpointId = DebugId<BreakpointIdTag>;

enum class DebugThreadState : u8 {
    Running,
    Paused,
    Exited,
};

enum class DebugScopeKind : u8 {
    Locals,
    Upvalues,
    Globals,
    Varargs,
    Exception,
};

enum class DebugVariableFilter : u8 {
    All,
    Indexed,
    Named,
};

enum class DebugStepMode : u8 {
    In,
    Over,
    Out,
};

enum class DebugExceptionCategory : u8 {
    RuntimeError,
    ResourceError,
    HostCancellation,
};

struct DebugExceptionInfo {
    Str exceptionId;
    Str description;
    Str breakMode = "always";
    DebugExceptionCategory category = DebugExceptionCategory::RuntimeError;
};

struct SourceLocation {
    SourceId sourceId;
    i32 line = 0;
    i32 column = 0;
    usize pc = 0;
};

struct DebugThread {
    ThreadId id;
    Str name;
    DebugThreadState state = DebugThreadState::Running;
};

struct DebugState {
    StateId id;
    ThreadId threadId;
    Str name;
    Str label;
    DebugThreadState state = DebugThreadState::Running;
    bool selected = false;
};

struct DebugStackFrame {
    FrameId id;
    ThreadId threadId;
    Str name;
    SourceLocation location;
    bool native = false;
};

struct DebugScope {
    Str name;
    DebugScopeKind kind = DebugScopeKind::Locals;
    VariableReference variablesReference;
    bool expensive = false;
};

struct DebugVariable {
    Str name;
    Str value;
    Str type;
    Opt<Str> evaluateName;
    VariableReference variablesReference;
    usize namedVariables = 0;
    usize indexedVariables = 0;
};

struct DebugResourceLimits {
    usize maxStackFrames = 256;
    usize maxVariablePageSize = 100;
    usize maxStringLength = 256;
    usize maxObjectHandles = 4096;
    usize maxExpressionLength = 4096;
    usize maxEvaluationDepth = 64;
    usize maxEvaluationSteps = 256;
    usize maxStates = 256;
};

enum class DebugErrorCode : u8 {
    InvalidState,
    InvalidReference,
    StaleReference,
    ResourceLimit,
    Timeout,
    Unsupported,
    RuntimeFailure,
};

struct DebugError {
    DebugErrorCode code = DebugErrorCode::RuntimeFailure;
    Str message;
    bool retryable = false;
};

template <typename T> using DebugResult = std::expected<T, DebugError>;

} // namespace Lua::Debugger
