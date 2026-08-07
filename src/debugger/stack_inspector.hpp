#pragma once

/**
 * @file stack_inspector.hpp
 * @brief Owner-thread stack, scope, and raw-value inspection while paused.
 */

#include "debugger/breakpoint_manager.hpp"
#include "debugger/pause_handles.hpp"
#include "core/value.hpp"

namespace Lua {
class Function;
class LuaState;
class Table;
} // namespace Lua

namespace Lua::Debugger {

class StackInspector {
public:
    explicit StackInspector(BreakpointManager& breakpoints, DebugResourceLimits limits = {});

    void beginPause(LuaState& state, PauseGeneration generation, ThreadId threadId = ThreadId{1},
                    StrView threadName = "main", const Value* exceptionValue = nullptr);
    void endPause() noexcept;

    /**
     * Object pointers retained by handles are valid only while the VM owner is
     * blocked in the debugger pause loop. That loop executes no Lua code and no
     * GC operation; endPause invalidates every handle before bytecode resumes.
     */

    [[nodiscard]] bool paused() const noexcept;
    [[nodiscard]] DebugResult<Vec<DebugThread>> threads() const;
    [[nodiscard]] DebugResult<Vec<DebugStackFrame>> stackTrace(ThreadId thread, usize startFrame, usize levels);
    [[nodiscard]] DebugResult<Vec<DebugScope>> scopes(FrameId frame);
    [[nodiscard]] DebugResult<Vec<DebugVariable>>
    variables(VariableReference reference, usize start, usize count,
              DebugVariableFilter filter = DebugVariableFilter::All);
    [[nodiscard]] DebugResult<DebugVariable> evaluate(FrameId frame, StrView expression);
    [[nodiscard]] DebugResult<DebugVariable> evaluateWithSideEffects(FrameId frame, StrView expression);
    [[nodiscard]] DebugResult<DebugVariable> setVariable(VariableReference reference, StrView name,
                                                         StrView valueExpression);

private:
    struct FrameDescriptor {
        LuaState* state = nullptr;
        usize callInfoIndex = 0;
        usize pc = 0;
        bool native = false;
        bool tailPlaceholder = false;
    };

    enum class VariableNodeKind : u8 {
        Locals,
        Upvalues,
        TableRaw,
        TableOverview,
        TableArray,
        TableHash,
        Exception,
    };

    struct VariableNode {
        VariableNodeKind kind = VariableNodeKind::Locals;
        FrameId frame;
        Table* table = nullptr;
    };

    struct RawEvaluatedValue {
        Value value;
        Opt<Str> stringLiteral;
    };

    [[nodiscard]] DebugResult<void> ensureFrames();
    [[nodiscard]] DebugResult<VariableReference> addNode(VariableNode node);
    [[nodiscard]] DebugResult<Vec<DebugVariable>> localVariables(FrameId frameId, const FrameDescriptor& frame);
    [[nodiscard]] DebugResult<Vec<DebugVariable>> upvalueVariables(FrameId frameId, const FrameDescriptor& frame);
    [[nodiscard]] DebugResult<Vec<DebugVariable>> tableVariables(Table& table, usize start, usize count,
                                                                 DebugVariableFilter filter);
    [[nodiscard]] DebugResult<Vec<DebugVariable>> tableOverview(const VariableNode& node);
    [[nodiscard]] DebugResult<VariableReference> tableSectionReference(Table& table, FrameId originFrame,
                                                                       VariableNodeKind kind);
    [[nodiscard]] Str weakTableSummary(const Table& table) const;
    [[nodiscard]] DebugResult<RawEvaluatedValue> evaluateRaw(FrameId frame, StrView expression);
    [[nodiscard]] DebugResult<Value> materializeValue(RawEvaluatedValue evaluated);
    [[nodiscard]] DebugResult<DebugVariable> makeVariable(Str name, const Value& value, FrameId originFrame = {});
    [[nodiscard]] DebugResult<VariableReference> tableReference(Table& table, FrameId originFrame = {});
    [[nodiscard]] Str formatValue(const Value& value) const;
    [[nodiscard]] Str formatString(StrView value) const;
    [[nodiscard]] Str valueTypeName(const Value& value) const;
    [[nodiscard]] Str formatTableKey(const Value& key) const;

    BreakpointManager& breakpoints_;
    DebugResourceLimits limits_;
    LuaState* state_ = nullptr;
    PauseGeneration generation_;
    ThreadId threadId_{1};
    Str threadName_ = "main";
    PauseHandleTable<FrameId, FrameDescriptor> frames_;
    PauseHandleTable<VariableReference, VariableNode> variables_;
    Vec<DebugStackFrame> stackFrames_;
    HashMap<u64, Vec<DebugScope>> scopesByFrame_;
    HashMap<const Table*, VariableReference> tableReferences_;
    HashMap<const Table*, VariableReference> tableArrayReferences_;
    HashMap<const Table*, VariableReference> tableHashReferences_;
    const Value* exceptionValueStorage_ = nullptr;
    bool framesBuilt_ = false;
};

} // namespace Lua::Debugger
