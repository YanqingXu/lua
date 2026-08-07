/**
 * @file stack_inspector.cpp
 * @brief Safe paused-state inspection without Lua callbacks or metamethods.
 */

#include "debugger/stack_inspector.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/thread.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "core/value.hpp"
#include "runtime/execution_policy.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/call_info.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/stack.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace Lua::Debugger {

namespace {

DebugError inspectorError(DebugErrorCode code, Str message) {
    return {code, std::move(message)};
}

} // namespace

StackInspector::StackInspector(BreakpointManager& breakpoints, DebugResourceLimits limits)
    : breakpoints_(breakpoints), limits_(limits) {}

void StackInspector::beginPause(LuaState& state, PauseGeneration generation, ThreadId threadId, StrView threadName,
                                const Value* exceptionValue) {
    state_ = &state;
    generation_ = generation;
    threadId_ = threadId;
    threadName_ = threadName;
    frames_.beginPause(generation);
    variables_.beginPause(generation);
    stackFrames_.clear();
    scopesByFrame_.clear();
    tableReferences_.clear();
    tableArrayReferences_.clear();
    tableHashReferences_.clear();
    exceptionValueStorage_ = exceptionValue;
    framesBuilt_ = false;
}

void StackInspector::endPause() noexcept {
    frames_.endPause();
    variables_.endPause();
    stackFrames_.clear();
    scopesByFrame_.clear();
    tableReferences_.clear();
    tableArrayReferences_.clear();
    tableHashReferences_.clear();
    exceptionValueStorage_ = nullptr;
    state_ = nullptr;
    generation_ = {};
    threadId_ = ThreadId{1};
    threadName_ = "main";
    framesBuilt_ = false;
}

bool StackInspector::paused() const noexcept {
    return state_ != nullptr && generation_.valid();
}

DebugResult<Vec<DebugThread>> StackInspector::threads() const {
    if (!paused()) {
        return std::unexpected(inspectorError(DebugErrorCode::InvalidState, "threads require a suspended VM"));
    }
    return Vec<DebugThread>{{threadId_, threadName_, DebugThreadState::Paused}};
}

DebugResult<void> StackInspector::ensureFrames() {
    if (!paused()) {
        return std::unexpected(inspectorError(DebugErrorCode::InvalidState, "stackTrace requires a suspended VM"));
    }
    if (framesBuilt_) {
        return {};
    }

    Stack& stack = state_->getStack();
    LuaVector<CallInfo>& callStack = state_->getCallStack();
    usize emitted = 0;
    for (usize callIndex = state_->getCurrentCI() + 1; callIndex-- > 0 && emitted < limits_.maxStackFrames;) {
        const CallInfo& call = callStack[callIndex];
        if (call.func >= stack.size() || !stack[call.func].isFunction()) {
            continue;
        }

        Function* function = stack[call.func].asFunction();
        FrameDescriptor descriptor{state_, callIndex, 0, function->isCFunction(), false};
        DebugStackFrame frame;
        frame.threadId = threadId_;
        frame.native = function->isCFunction();
        frame.name = frame.native ? "C function" : "Lua function";

        if (!frame.native && function->getProto() != nullptr) {
            Proto* proto = function->getProto();
            const auto code = proto->getInstructionSpan();
            if (call.savedpc != nullptr && !code.empty() && call.savedpc > code.data() &&
                call.savedpc <= code.data() + code.size()) {
                descriptor.pc = static_cast<usize>(call.savedpc - code.data() - 1);
            }
            if (proto->getSource() != nullptr) {
                frame.location.sourceId = breakpoints_.registerSourceName(proto->getSource()->view());
                frame.name = proto->getSource()->view();
            }
            frame.location.pc = descriptor.pc;
            frame.location.line = descriptor.pc < proto->getLineInfo().size() ? proto->getLine(descriptor.pc) : 0;
        }

        DebugResult<FrameId> frameId = frames_.add(descriptor);
        if (!frameId) {
            return std::unexpected(frameId.error());
        }
        frame.id = *frameId;
        stackFrames_.push_back(std::move(frame));
        ++emitted;

        for (i32 tail = 0; tail < call.tailcalls && emitted < limits_.maxStackFrames; ++tail) {
            FrameDescriptor tailDescriptor{state_, callIndex, descriptor.pc, false, true};
            DebugResult<FrameId> tailId = frames_.add(tailDescriptor);
            if (!tailId) {
                return std::unexpected(tailId.error());
            }
            stackFrames_.push_back(DebugStackFrame{*tailId, threadId_, "[tail call]", {}, false});
            ++emitted;
        }
    }
    framesBuilt_ = true;
    return {};
}

DebugResult<Vec<DebugStackFrame>> StackInspector::stackTrace(ThreadId thread, usize startFrame, usize levels) {
    if (thread != threadId_) {
        return std::unexpected(inspectorError(DebugErrorCode::InvalidReference, "unknown debug thread"));
    }
    if (DebugResult<void> built = ensureFrames(); !built) {
        return std::unexpected(built.error());
    }

    if (startFrame >= stackFrames_.size()) {
        return Vec<DebugStackFrame>{};
    }
    const usize requested = levels == 0 ? limits_.maxStackFrames : std::min(levels, limits_.maxStackFrames);
    const usize end = std::min(stackFrames_.size(), startFrame + requested);
    return Vec<DebugStackFrame>(stackFrames_.begin() + static_cast<isize>(startFrame),
                                stackFrames_.begin() + static_cast<isize>(end));
}

DebugResult<Vec<DebugScope>> StackInspector::scopes(FrameId frame) {
    if (DebugResult<void> built = ensureFrames(); !built) {
        return std::unexpected(built.error());
    }
    const DebugResult<std::reference_wrapper<const FrameDescriptor>> descriptor = frames_.lookup(frame);
    if (!descriptor) {
        return std::unexpected(descriptor.error());
    }
    if (descriptor->get().tailPlaceholder) {
        return Vec<DebugScope>{};
    }
    if (const auto cached = scopesByFrame_.find(frame.value()); cached != scopesByFrame_.end()) {
        return cached->second;
    }

    Vec<DebugScope> result;
    const auto addScope = [&](Str name, DebugScopeKind kind, VariableNode node) -> DebugResult<void> {
        DebugResult<VariableReference> reference = addNode(node);
        if (!reference) {
            return std::unexpected(reference.error());
        }
        result.push_back(DebugScope{std::move(name), kind, *reference, false});
        return {};
    };

    const bool exceptionFrame =
        exceptionValueStorage_ != nullptr && descriptor->get().callInfoIndex == state_->getCurrentCI();
    if (descriptor->get().native) {
        if (exceptionFrame) {
            if (DebugResult<void> exception =
                    addScope("Exception", DebugScopeKind::Exception, VariableNode{VariableNodeKind::Exception, frame});
                !exception) {
                return std::unexpected(exception.error());
            }
        }
        scopesByFrame_.emplace(frame.value(), result);
        return result;
    }

    if (DebugResult<void> local =
            addScope("Locals", DebugScopeKind::Locals, VariableNode{VariableNodeKind::Locals, frame, nullptr});
        !local) {
        return std::unexpected(local.error());
    }
    if (DebugResult<void> upvalues =
            addScope("Upvalues", DebugScopeKind::Upvalues, VariableNode{VariableNodeKind::Upvalues, frame, nullptr});
        !upvalues) {
        return std::unexpected(upvalues.error());
    }
    if (DebugResult<void> globals = addScope("Globals", DebugScopeKind::Globals,
                                             VariableNode{VariableNodeKind::TableRaw, frame,
                                                          state_->getGlobalTable()});
        !globals) {
        return std::unexpected(globals.error());
    }
    if (exceptionFrame) {
        if (DebugResult<void> exception =
                addScope("Exception", DebugScopeKind::Exception, VariableNode{VariableNodeKind::Exception, frame});
            !exception) {
            return std::unexpected(exception.error());
        }
    }

    scopesByFrame_.emplace(frame.value(), result);
    return result;
}

DebugResult<Vec<DebugVariable>> StackInspector::variables(VariableReference reference, usize start, usize count,
                                                          DebugVariableFilter filter) {
    const DebugResult<std::reference_wrapper<const VariableNode>> node = variables_.lookup(reference);
    if (!node) {
        return std::unexpected(node.error());
    }
    switch (node->get().kind) {
    case VariableNodeKind::Locals: {
        const auto frame = frames_.lookup(node->get().frame);
        return frame ? localVariables(node->get().frame, frame->get())
                     : DebugResult<Vec<DebugVariable>>(std::unexpected(frame.error()));
    }
    case VariableNodeKind::Upvalues: {
        const auto frame = frames_.lookup(node->get().frame);
        return frame ? upvalueVariables(node->get().frame, frame->get())
                     : DebugResult<Vec<DebugVariable>>(std::unexpected(frame.error()));
    }
    case VariableNodeKind::TableRaw:
        if (node->get().table == nullptr) {
            return std::unexpected(inspectorError(DebugErrorCode::InvalidReference, "table handle is empty"));
        }
        return tableVariables(*node->get().table, start, count, filter);
    case VariableNodeKind::TableOverview:
        if (node->get().table == nullptr) {
            return std::unexpected(inspectorError(DebugErrorCode::InvalidReference, "table handle is empty"));
        }
        if (filter == DebugVariableFilter::All) {
            return tableOverview(node->get());
        }
        return tableVariables(*node->get().table, start, count, filter);
    case VariableNodeKind::TableArray:
        if (node->get().table == nullptr) {
            return std::unexpected(inspectorError(DebugErrorCode::InvalidReference, "table handle is empty"));
        }
        return tableVariables(*node->get().table, start, count, DebugVariableFilter::Indexed);
    case VariableNodeKind::TableHash:
        if (node->get().table == nullptr) {
            return std::unexpected(inspectorError(DebugErrorCode::InvalidReference, "table handle is empty"));
        }
        return tableVariables(*node->get().table, start, count, DebugVariableFilter::Named);
    case VariableNodeKind::Exception: {
        if (exceptionValueStorage_ == nullptr) {
            return std::unexpected(inspectorError(DebugErrorCode::StaleReference, "exception value is unavailable"));
        }
        DebugResult<DebugVariable> value = makeVariable("error", *exceptionValueStorage_);
        if (!value) {
            return std::unexpected(value.error());
        }
        return Vec<DebugVariable>{std::move(*value)};
    }
    }
    return std::unexpected(inspectorError(DebugErrorCode::Unsupported, "unsupported variable node"));
}

DebugResult<StackInspector::RawEvaluatedValue> StackInspector::evaluateRaw(FrameId frameId, StrView expression) {
    if (expression.empty() || expression.size() > limits_.maxExpressionLength) {
        return std::unexpected(
            inspectorError(DebugErrorCode::ResourceLimit, "expression is empty or exceeds the configured limit"));
    }
    if (DebugResult<void> built = ensureFrames(); !built) {
        return std::unexpected(built.error());
    }
    const auto descriptorResult = frames_.lookup(frameId);
    if (!descriptorResult) {
        return std::unexpected(descriptorResult.error());
    }
    const FrameDescriptor& frame = descriptorResult->get();
    if (frame.native || frame.tailPlaceholder || frame.state == nullptr) {
        return std::unexpected(inspectorError(DebugErrorCode::Unsupported,
                                              "read-only evaluation requires a live Lua stack frame"));
    }

    const auto error = [](DebugErrorCode code, Str message) -> DebugResult<RawEvaluatedValue> {
        return std::unexpected(inspectorError(code, std::move(message)));
    };
    usize position = 0;
    usize depth = 0;
    usize steps = 0;
    const auto consumeStep = [&]() -> bool {
        ++steps;
        return steps <= limits_.maxEvaluationSteps;
    };
    const auto skipWhitespace = [&]() {
        while (position < expression.size() &&
               std::isspace(static_cast<unsigned char>(expression[position])) != 0) {
            ++position;
        }
    };
    const auto isIdentifierStart = [](char character) {
        const unsigned char value = static_cast<unsigned char>(character);
        return std::isalpha(value) != 0 || character == '_';
    };
    const auto isIdentifierPart = [&](char character) {
        const unsigned char value = static_cast<unsigned char>(character);
        return std::isalnum(value) != 0 || character == '_';
    };
    const auto parseIdentifier = [&]() -> Opt<Str> {
        skipWhitespace();
        if (position >= expression.size() || !isIdentifierStart(expression[position])) {
            return {};
        }
        const usize start = position++;
        while (position < expression.size() && isIdentifierPart(expression[position])) {
            ++position;
        }
        return Str(expression.substr(start, position - start));
    };
    const auto parseString = [&]() -> DebugResult<RawEvaluatedValue> {
        const char quote = expression[position++];
        Str decoded;
        while (position < expression.size()) {
            char character = expression[position++];
            if (character == quote) {
                RawEvaluatedValue result;
                result.stringLiteral = std::move(decoded);
                return result;
            }
            if (character == '\\') {
                if (position >= expression.size()) {
                    return error(DebugErrorCode::Unsupported, "unterminated string escape in expression");
                }
                character = expression[position++];
                switch (character) {
                case 'n':
                    decoded.push_back('\n');
                    break;
                case 'r':
                    decoded.push_back('\r');
                    break;
                case 't':
                    decoded.push_back('\t');
                    break;
                case '\\':
                case '\'':
                case '"':
                    decoded.push_back(character);
                    break;
                default:
                    return error(DebugErrorCode::Unsupported, "unsupported string escape in expression");
                }
            } else {
                decoded.push_back(character);
            }
            if (decoded.size() > limits_.maxExpressionLength) {
                return error(DebugErrorCode::ResourceLimit, "string literal exceeds the configured limit");
            }
        }
        return error(DebugErrorCode::Unsupported, "unterminated string literal in expression");
    };

    const auto tableStringValue = [&](Table* table, StrView key) -> Value {
        if (table == nullptr) {
            return Value();
        }
        GCString* interned = frame.state->getGlobalState().getStringPool().find(key);
        return interned == nullptr ? Value() : table->get(Value(interned));
    };
    const auto resolveName = [&](StrView name) -> DebugResult<RawEvaluatedValue> {
        if (!consumeStep()) {
            return error(DebugErrorCode::ResourceLimit, "evaluation step limit exceeded");
        }
        Stack& stack = frame.state->getStack();
        LuaVector<CallInfo>& calls = frame.state->getCallStack();
        if (frame.callInfoIndex >= calls.size()) {
            return error(DebugErrorCode::StaleReference, "evaluation frame is no longer available");
        }
        const CallInfo& call = calls[frame.callInfoIndex];
        if (call.func >= stack.size() || !stack[call.func].isFunction()) {
            return error(DebugErrorCode::StaleReference, "evaluation frame function is no longer available");
        }
        Function* function = stack[call.func].asFunction();
        Proto* proto = function == nullptr ? nullptr : function->getProto();
        const LocVar* selected = nullptr;
        usize selectedIndex = 0;
        if (proto != nullptr) {
            for (usize index = 0; index < proto->getLocVarCount(); ++index) {
                const LocVar& local = proto->getLocVar(index);
                if (local.varname == nullptr || local.varname->view() != name || local.reg < 0 ||
                    frame.pc < static_cast<usize>(std::max(local.startpc, 0)) ||
                    frame.pc >= static_cast<usize>(std::max(local.endpc, 0))) {
                    continue;
                }
                if (selected == nullptr || local.startpc > selected->startpc ||
                    (local.startpc == selected->startpc && index > selectedIndex)) {
                    selected = &local;
                    selectedIndex = index;
                }
            }
        }
        if (selected != nullptr) {
            const usize slot = call.base + static_cast<usize>(selected->reg);
            if (slot < stack.size() && slot < call.top) {
                return RawEvaluatedValue{stack[slot], {}};
            }
        }
        if (function != nullptr) {
            for (usize index = 0; index < function->getUpvalueCount(); ++index) {
                if (proto == nullptr || index >= proto->getUpvalueNameCount() ||
                    proto->getUpvalueName(index) == nullptr || proto->getUpvalueName(index)->view() != name) {
                    continue;
                }
                Upvalue* upvalue = function->getUpvalue(index);
                if (upvalue != nullptr) {
                    return RawEvaluatedValue{upvalue->getValue(stack), {}};
                }
            }
            const Value environmentValue = tableStringValue(function->getEnv(), name);
            if (!environmentValue.isNil()) {
                return RawEvaluatedValue{environmentValue, {}};
            }
        }
        return RawEvaluatedValue{tableStringValue(frame.state->getGlobalTable(), name), {}};
    };
    const auto rawIndex = [&](const RawEvaluatedValue& object,
                              const RawEvaluatedValue& key) -> DebugResult<RawEvaluatedValue> {
        if (!consumeStep()) {
            return error(DebugErrorCode::ResourceLimit, "evaluation step limit exceeded");
        }
        if (!object.value.isTable() || object.value.asTable() == nullptr || object.stringLiteral) {
            return error(DebugErrorCode::Unsupported, "read-only indexing is supported only for tables");
        }
        if (key.stringLiteral) {
            return RawEvaluatedValue{tableStringValue(object.value.asTable(), *key.stringLiteral), {}};
        }
        if (key.value.isNil()) {
            return error(DebugErrorCode::Unsupported, "nil cannot be used as a table index");
        }
        return RawEvaluatedValue{object.value.asTable()->get(key.value), {}};
    };

    Func<DebugResult<RawEvaluatedValue>()> parseExpression;
    parseExpression = [&]() -> DebugResult<RawEvaluatedValue> {
        ++depth;
        struct DepthGuard {
            usize& value;
            ~DepthGuard() {
                --value;
            }
        } depthGuard{depth};
        if (depth > limits_.maxEvaluationDepth || !consumeStep()) {
            return error(DebugErrorCode::ResourceLimit, "evaluation depth or step limit exceeded");
        }

        skipWhitespace();
        if (position >= expression.size()) {
            return error(DebugErrorCode::Unsupported, "expected a read-only expression");
        }
        RawEvaluatedValue current;
        const char first = expression[position];
        if (first == '(') {
            ++position;
            auto nested = parseExpression();
            if (!nested) {
                return nested;
            }
            skipWhitespace();
            if (position >= expression.size() || expression[position++] != ')') {
                return error(DebugErrorCode::Unsupported, "missing closing parenthesis in expression");
            }
            current = std::move(*nested);
        } else if (first == '\'' || first == '"') {
            auto literal = parseString();
            if (!literal) {
                return literal;
            }
            current = std::move(*literal);
        } else if (std::isdigit(static_cast<unsigned char>(first)) != 0 ||
                   (first == '-' && position + 1 < expression.size() &&
                    std::isdigit(static_cast<unsigned char>(expression[position + 1])) != 0)) {
            const usize start = position;
            if (expression[position] == '-') {
                ++position;
            }
            while (position < expression.size() &&
                   std::isdigit(static_cast<unsigned char>(expression[position])) != 0) {
                ++position;
            }
            if (position < expression.size() && expression[position] == '.') {
                ++position;
                while (position < expression.size() &&
                       std::isdigit(static_cast<unsigned char>(expression[position])) != 0) {
                    ++position;
                }
            }
            if (position < expression.size() && (expression[position] == 'e' || expression[position] == 'E')) {
                ++position;
                if (position < expression.size() && (expression[position] == '+' || expression[position] == '-')) {
                    ++position;
                }
                while (position < expression.size() &&
                       std::isdigit(static_cast<unsigned char>(expression[position])) != 0) {
                    ++position;
                }
            }
            const Str token(expression.substr(start, position - start));
            char* end = nullptr;
            errno = 0;
            const f64 number = std::strtod(token.c_str(), &end);
            if (errno == ERANGE || end != token.c_str() + token.size() || !std::isfinite(number)) {
                return error(DebugErrorCode::Unsupported, "invalid numeric literal in expression");
            }
            current.value = Value(number);
        } else {
            Opt<Str> identifier = parseIdentifier();
            if (!identifier) {
                return error(DebugErrorCode::Unsupported,
                             "only identifiers, literals, parentheses, and raw table indexing are supported");
            }
            if (*identifier == "nil") {
                current.value = Value();
            } else if (*identifier == "true") {
                current.value = Value(true);
            } else if (*identifier == "false") {
                current.value = Value(false);
            } else {
                auto resolved = resolveName(*identifier);
                if (!resolved) {
                    return resolved;
                }
                current = std::move(*resolved);
            }
        }

        for (;;) {
            skipWhitespace();
            if (position < expression.size() && expression[position] == '.') {
                ++position;
                Opt<Str> field = parseIdentifier();
                if (!field) {
                    return error(DebugErrorCode::Unsupported, "field access requires an identifier");
                }
                RawEvaluatedValue key;
                key.stringLiteral = std::move(*field);
                auto indexed = rawIndex(current, key);
                if (!indexed) {
                    return indexed;
                }
                current = std::move(*indexed);
                continue;
            }
            if (position < expression.size() && expression[position] == '[') {
                ++position;
                auto key = parseExpression();
                if (!key) {
                    return key;
                }
                skipWhitespace();
                if (position >= expression.size() || expression[position++] != ']') {
                    return error(DebugErrorCode::Unsupported, "missing closing bracket in expression");
                }
                auto indexed = rawIndex(current, *key);
                if (!indexed) {
                    return indexed;
                }
                current = std::move(*indexed);
                continue;
            }
            break;
        }
        return current;
    };

    auto evaluated = parseExpression();
    if (!evaluated) {
        return std::unexpected(evaluated.error());
    }
    skipWhitespace();
    if (position != expression.size()) {
        return std::unexpected(inspectorError(
            DebugErrorCode::Unsupported,
            "read-only evaluation rejects calls, assignments, operators, and other side-effecting syntax"));
    }
    return *evaluated;
}

DebugResult<Value> StackInspector::materializeValue(RawEvaluatedValue evaluated) {
    if (evaluated.stringLiteral) {
        if (state_ == nullptr) {
            return std::unexpected(inspectorError(DebugErrorCode::InvalidState, "value materialization requires a pause"));
        }
        return Value(state_->getGlobalState().getStringPool().intern(*evaluated.stringLiteral));
    }
    return evaluated.value;
}

DebugResult<DebugVariable> StackInspector::evaluate(FrameId frameId, StrView expression) {
    auto evaluated = evaluateRaw(frameId, expression);
    if (!evaluated) {
        return std::unexpected(evaluated.error());
    }
    if (evaluated->stringLiteral) {
        DebugVariable result;
        result.name = "result";
        result.value = formatString(*evaluated->stringLiteral);
        result.type = "string";
        result.evaluateName = Str(expression);
        return result;
    }
    DebugResult<DebugVariable> result = makeVariable("result", evaluated->value, frameId);
    if (result) {
        result->evaluateName = Str(expression);
    }
    return result;
}

DebugResult<DebugVariable> StackInspector::evaluateWithSideEffects(FrameId frameId, StrView expression) {
    if (expression.empty() || expression.size() > limits_.maxExpressionLength) {
        return std::unexpected(
            inspectorError(DebugErrorCode::ResourceLimit, "expression is empty or exceeds the configured limit"));
    }
    if (DebugResult<void> built = ensureFrames(); !built) {
        return std::unexpected(built.error());
    }
    const auto descriptorResult = frames_.lookup(frameId);
    if (!descriptorResult) {
        return std::unexpected(descriptorResult.error());
    }
    const FrameDescriptor frame = descriptorResult->get();
    if (frame.native || frame.tailPlaceholder || frame.state == nullptr) {
        return std::unexpected(inspectorError(DebugErrorCode::Unsupported,
                                              "side-effecting evaluation requires a live Lua stack frame"));
    }
    LuaState& luaState = *frame.state;
    if (frame.callInfoIndex >= luaState.getCallStack().size()) {
        return std::unexpected(inspectorError(DebugErrorCode::StaleReference, "evaluation frame is unavailable"));
    }
    Stack& stack = luaState.getStack();
    const CallInfo& call = luaState.getCallStack()[frame.callInfoIndex];
    if (call.func >= stack.size() || !stack[call.func].isFunction()) {
        return std::unexpected(inspectorError(DebugErrorCode::StaleReference,
                                              "evaluation frame function is unavailable"));
    }
    Function* targetFunction = stack[call.func].asFunction();
    Proto* targetProto = targetFunction == nullptr ? nullptr : targetFunction->getProto();
    if (targetFunction == nullptr || targetProto == nullptr) {
        return std::unexpected(inspectorError(DebugErrorCode::Unsupported,
                                              "side-effecting evaluation is unavailable for native frames"));
    }

    RuntimeServices services(luaState.getGlobalState());
    Table* environment = services.gc.create<Table>();
    const usize originalTop = luaState.getAbsoluteTop();
    const usize originalCallCount = luaState.getCallStackSize();
    Vec<Value> trailingSlots;
    trailingSlots.reserve(stack.size() > originalTop ? stack.size() - originalTop : 0);
    for (usize index = originalTop; index < stack.size(); ++index) {
        trailingSlots.push_back(stack[index]);
    }
    luaState.pushTable(environment);

    ExecutionPolicy& executionPolicy = luaState.getGlobalState().getExecutionPolicy();
    ExecutionPolicy::Limits savedLimits;
    savedLimits.instructionBudget = executionPolicy.remainingInstructions();
    savedLimits.deadline = executionPolicy.deadline();
    savedLimits.finalizerBudgetPerDrain = executionPolicy.finalizerBudgetPerDrain();
    savedLimits.nativeWorkBudget = executionPolicy.remainingNativeWork();
    struct EvaluationGuard {
        LuaState& state;
        usize originalTop;
        usize originalCallCount;
        Vec<Value>& trailingSlots;
        ExecutionPolicy& policy;
        ExecutionPolicy::Limits savedLimits;

        ~EvaluationGuard() {
            while (state.getCallStackSize() > originalCallCount) {
                state.popCallInfo();
            }
            Stack& stack = state.getStack();
            for (usize index = 0; index < trailingSlots.size(); ++index) {
                stack[originalTop + index] = trailingSlots[index];
            }
            for (usize index = originalTop + trailingSlots.size(); index < stack.size(); ++index) {
                stack[index] = Value();
            }
            state.setAbsoluteTop(originalTop);
            policy.configure(savedLimits);
        }
    } guard{luaState, originalTop, originalCallCount, trailingSlots, executionPolicy, savedLimits};

    struct EnvironmentBinding {
        Value key;
        Table* destination = nullptr;
    };
    struct LexicalBinding {
        Str name;
        bool local = false;
        usize slot = 0;
        Upvalue* upvalue = nullptr;
    };
    Vec<EnvironmentBinding> environmentBindings;
    Vec<LexicalBinding> lexicalBindings;
    const auto findEnvironmentBinding = [&](const Value& key) -> EnvironmentBinding* {
        for (EnvironmentBinding& binding : environmentBindings) {
            if (binding.key == key) {
                return &binding;
            }
        }
        return nullptr;
    };
    const auto copyEnvironment = [&](Table* source) -> DebugResult<void> {
        if (source == nullptr) {
            return {};
        }
        Value key;
        Value nextKey;
        Value nextValue;
        while (source->next(key, nextKey, nextValue)) {
            EnvironmentBinding* existing = findEnvironmentBinding(nextKey);
            if (existing == nullptr) {
                if (environmentBindings.size() >= limits_.maxSideEffectEnvironmentEntries) {
                    return std::unexpected(inspectorError(DebugErrorCode::ResourceLimit,
                                                          "evaluation environment exceeds the configured entry limit"));
                }
                environmentBindings.push_back({nextKey, source});
            } else {
                existing->destination = source;
            }
            environment->set(nextKey, nextValue);
            key = nextKey;
        }
        return {};
    };
    Table* globals = luaState.getGlobalTable();
    Table* functionEnvironment = targetFunction->getEnv() == nullptr ? globals : targetFunction->getEnv();
    try {
        if (auto copied = copyEnvironment(globals); !copied) {
            return std::unexpected(copied.error());
        }
        if (functionEnvironment != globals) {
            if (auto copied = copyEnvironment(functionEnvironment); !copied) {
                return std::unexpected(copied.error());
            }
        }
    } catch (const RuntimeError& error) {
        return std::unexpected(inspectorError(DebugErrorCode::RuntimeFailure, error.what()));
    }

    const auto installLexical = [&](Str name, bool local, usize slot, Upvalue* upvalue, const Value& value) {
        for (LexicalBinding& binding : lexicalBindings) {
            if (binding.name == name) {
                binding = LexicalBinding{std::move(name), local, slot, upvalue};
                environment->set(Value(services.strings.intern(binding.name)), value);
                return;
            }
        }
        lexicalBindings.push_back({std::move(name), local, slot, upvalue});
        LexicalBinding& binding = lexicalBindings.back();
        environment->set(Value(services.strings.intern(binding.name)), value);
    };
    try {
        for (usize index = 0; index < targetFunction->getUpvalueCount(); ++index) {
            Upvalue* upvalue = targetFunction->getUpvalue(index);
            if (upvalue == nullptr) {
                continue;
            }
            Str name = "(upvalue " + std::to_string(index + 1) + ")";
            if (index < targetProto->getUpvalueNameCount() && targetProto->getUpvalueName(index) != nullptr) {
                name = targetProto->getUpvalueName(index)->view();
            }
            if (!name.empty() && name.front() != '(') {
                installLexical(std::move(name), false, 0, upvalue, upvalue->getValue(stack));
            }
        }
        for (usize index = 0; index < targetProto->getLocVarCount(); ++index) {
            const LocVar& local = targetProto->getLocVar(index);
            if (local.varname == nullptr || local.reg < 0 || frame.pc < static_cast<usize>(std::max(local.startpc, 0)) ||
                frame.pc >= static_cast<usize>(std::max(local.endpc, 0))) {
                continue;
            }
            const usize slot = call.base + static_cast<usize>(local.reg);
            if (slot < stack.size() && slot < call.top) {
                installLexical(Str(local.varname->view()), true, slot, nullptr, stack[slot]);
            }
        }
    } catch (const RuntimeError& error) {
        return std::unexpected(inspectorError(DebugErrorCode::RuntimeFailure, error.what()));
    }

    const auto isLexicalKey = [&](const Value& key) {
        if (!key.isString() || key.asString() == nullptr) {
            return false;
        }
        for (const LexicalBinding& binding : lexicalBindings) {
            if (key.asString()->view() == binding.name) {
                return true;
            }
        }
        return false;
    };
    const auto writeBack = [&]() -> DebugResult<void> {
        try {
            Stack& liveStack = luaState.getStack();
            for (const LexicalBinding& binding : lexicalBindings) {
                const Value key(services.strings.intern(binding.name));
                const Value value = environment->get(key);
                if (binding.local) {
                    if (binding.slot >= liveStack.size()) {
                        return std::unexpected(inspectorError(DebugErrorCode::StaleReference,
                                                              "local writeback slot is unavailable"));
                    }
                    liveStack[binding.slot] = value;
                } else if (binding.upvalue != nullptr) {
                    binding.upvalue->setValue(liveStack, value);
                }
            }
            for (const EnvironmentBinding& binding : environmentBindings) {
                if (!isLexicalKey(binding.key) && binding.destination != nullptr) {
                    binding.destination->set(binding.key, environment->get(binding.key));
                }
            }
            Value key;
            Value nextKey;
            Value nextValue;
            usize count = 0;
            while (environment->next(key, nextKey, nextValue)) {
                if (++count > limits_.maxSideEffectEnvironmentEntries) {
                    return std::unexpected(inspectorError(DebugErrorCode::ResourceLimit,
                                                          "evaluation writeback exceeds the configured entry limit"));
                }
                if (!isLexicalKey(nextKey) && findEnvironmentBinding(nextKey) == nullptr) {
                    functionEnvironment->set(nextKey, nextValue);
                }
                key = nextKey;
            }
            return {};
        } catch (const RuntimeError& error) {
            return std::unexpected(inspectorError(DebugErrorCode::RuntimeFailure, error.what()));
        }
    };

    try {
        Parser parser("return (" + Str(expression) + ")", services);
        auto parsed = parser.parse();
        if (!parsed) {
            return std::unexpected(inspectorError(DebugErrorCode::Unsupported,
                                                  "Debug Console expression syntax error at line " +
                                                      std::to_string(parsed.error().getLine()) + ": " +
                                                      parsed.error().what()));
        }
        CodeGenerator generator(services);
        Proto* proto = generator.generate(*parsed, "=(debug console)");
        proto->setDebugName(services.strings.intern("<debug-console>"));
        Function* function = services.gc.create<Function>(proto);
        function->setEnv(environment);

        ExecutionPolicy::Limits evaluationLimits = savedLimits;
        evaluationLimits.instructionBudget =
            std::min<u64>(savedLimits.instructionBudget, limits_.maxSideEffectInstructions);
        const auto evaluationDeadline =
            ExecutionPolicy::Clock::now() + std::chrono::milliseconds(limits_.sideEffectTimeoutMs);
        evaluationLimits.deadline = std::min(savedLimits.deadline, evaluationDeadline);
        executionPolicy.configure(evaluationLimits);

        luaState.pushFunction(function);
        RuntimeServices evaluationServices(luaState.getGlobalState());
        evaluationServices.debugger = nullptr;
        VM::call(evaluationServices, &luaState, 0, 1);
        const Value resultValue = luaState.top();
        if (auto written = writeBack(); !written) {
            return std::unexpected(written.error());
        }
        DebugResult<DebugVariable> result = makeVariable("result", resultValue, frameId);
        if (result) {
            result->evaluateName = Str(expression);
        }
        return result;
    } catch (const RuntimeError& error) {
        (void)writeBack();
        const ExecutionStopReason reason = executionPolicy.lastStopReason();
        const DebugErrorCode code = reason == ExecutionStopReason::None ? DebugErrorCode::RuntimeFailure
                                                                        : DebugErrorCode::ResourceLimit;
        return std::unexpected(inspectorError(code, "Debug Console evaluation failed: " + Str(error.what())));
    } catch (const std::exception& error) {
        (void)writeBack();
        return std::unexpected(inspectorError(DebugErrorCode::RuntimeFailure,
                                              "Debug Console evaluation failed: " + Str(error.what())));
    }
}

DebugResult<DebugVariable> StackInspector::setVariable(VariableReference reference, StrView requestedName,
                                                        StrView valueExpression) {
    if (requestedName.empty() || requestedName.size() > limits_.maxExpressionLength) {
        return std::unexpected(inspectorError(DebugErrorCode::ResourceLimit,
                                              "variable name is empty or exceeds the configured limit"));
    }
    const auto nodeResult = variables_.lookup(reference);
    if (!nodeResult) {
        return std::unexpected(nodeResult.error());
    }
    const VariableNode& node = nodeResult->get();
    if (!node.frame.valid()) {
        return std::unexpected(inspectorError(DebugErrorCode::Unsupported,
                                              "variable value requires a live originating Lua frame"));
    }
    auto rawValue = evaluateRaw(node.frame, valueExpression);
    if (!rawValue) {
        return std::unexpected(rawValue.error());
    }
    auto valueResult = materializeValue(std::move(*rawValue));
    if (!valueResult) {
        return std::unexpected(valueResult.error());
    }
    const Value value = *valueResult;

    if (node.kind == VariableNodeKind::TableRaw || node.kind == VariableNodeKind::TableOverview ||
        node.kind == VariableNodeKind::TableArray || node.kind == VariableNodeKind::TableHash) {
        if (node.kind == VariableNodeKind::TableOverview &&
            (requestedName == "[array]" || requestedName == "[hash]" || requestedName == "[metatable]")) {
            return std::unexpected(inspectorError(DebugErrorCode::Unsupported,
                                                  "synthetic table groups cannot be assigned"));
        }
        if (node.table == nullptr) {
            return std::unexpected(inspectorError(DebugErrorCode::InvalidReference, "table handle is empty"));
        }
        Value key;
        if (requestedName.size() >= 2 && requestedName.front() == '[' && requestedName.back() == ']') {
            const StrView keyExpression = requestedName.substr(1, requestedName.size() - 2);
            auto rawKey = evaluateRaw(node.frame, keyExpression);
            if (!rawKey) {
                return std::unexpected(inspectorError(DebugErrorCode::Unsupported,
                                                      "table key cannot be reconstructed safely: " +
                                                          rawKey.error().message));
            }
            auto keyResult = materializeValue(std::move(*rawKey));
            if (!keyResult || keyResult->isNil() || keyResult->isTable() || keyResult->isFunction() ||
                keyResult->isUserdata() || keyResult->isThread() || keyResult->isLightUserdata()) {
                return std::unexpected(inspectorError(DebugErrorCode::Unsupported,
                                                      "only string, number, and boolean raw table keys are writable"));
            }
            key = *keyResult;
        } else {
            key = Value(state_->getGlobalState().getStringPool().intern(requestedName));
        }
        try {
            node.table->set(key, value);
        } catch (const RuntimeError& error) {
            return std::unexpected(inspectorError(DebugErrorCode::RuntimeFailure, error.what()));
        }
        return makeVariable(Str(requestedName), value, node.frame);
    }

    const auto frameResult = frames_.lookup(node.frame);
    if (!frameResult) {
        return std::unexpected(frameResult.error());
    }
    const FrameDescriptor& frame = frameResult->get();
    if (frame.state == nullptr || frame.callInfoIndex >= frame.state->getCallStack().size()) {
        return std::unexpected(inspectorError(DebugErrorCode::StaleReference, "variable frame is unavailable"));
    }
    Stack& stack = frame.state->getStack();
    const CallInfo& call = frame.state->getCallStack()[frame.callInfoIndex];
    if (call.func >= stack.size() || !stack[call.func].isFunction()) {
        return std::unexpected(inspectorError(DebugErrorCode::StaleReference, "frame function is unavailable"));
    }
    Function* function = stack[call.func].asFunction();
    Proto* proto = function == nullptr ? nullptr : function->getProto();

    if (node.kind == VariableNodeKind::Locals) {
        if (proto == nullptr) {
            return std::unexpected(inspectorError(DebugErrorCode::Unsupported, "native frames have no writable locals"));
        }
        HashMap<Str, usize> duplicateNames;
        for (usize index = 0; index < proto->getLocVarCount(); ++index) {
            const LocVar& local = proto->getLocVar(index);
            if (local.varname == nullptr || local.startpc < 0 || local.endpc <= local.startpc || local.reg < 0 ||
                frame.pc < static_cast<usize>(local.startpc) || frame.pc >= static_cast<usize>(local.endpc)) {
                continue;
            }
            const usize slot = call.base + static_cast<usize>(local.reg);
            if (slot >= stack.size() || slot >= call.top) {
                continue;
            }
            Str displayName(local.varname->view());
            const usize duplicate = ++duplicateNames[displayName];
            if (duplicate > 1) {
                displayName += " (" + std::to_string(duplicate) + ")";
            }
            if (displayName == requestedName) {
                stack[slot] = value;
                return makeVariable(std::move(displayName), value, node.frame);
            }
        }
        return std::unexpected(inspectorError(DebugErrorCode::InvalidReference, "visible local was not found"));
    }

    if (node.kind == VariableNodeKind::Upvalues) {
        for (usize index = 0; function != nullptr && index < function->getUpvalueCount(); ++index) {
            Upvalue* upvalue = function->getUpvalue(index);
            if (upvalue == nullptr) {
                continue;
            }
            Str displayName = "(upvalue " + std::to_string(index + 1) + ")";
            if (proto != nullptr && index < proto->getUpvalueNameCount() && proto->getUpvalueName(index) != nullptr) {
                displayName = proto->getUpvalueName(index)->view();
            }
            if (displayName == requestedName) {
                upvalue->setValue(stack, value);
                return makeVariable(std::move(displayName), value, node.frame);
            }
        }
        return std::unexpected(inspectorError(DebugErrorCode::InvalidReference, "upvalue was not found"));
    }

    return std::unexpected(inspectorError(DebugErrorCode::Unsupported, "this scope is read-only"));
}

DebugResult<VariableReference> StackInspector::addNode(VariableNode node) {
    if (variables_.size() >= limits_.maxObjectHandles) {
        return std::unexpected(inspectorError(DebugErrorCode::ResourceLimit, "debug object handle limit exceeded"));
    }
    return variables_.add(std::move(node));
}

DebugResult<Vec<DebugVariable>> StackInspector::localVariables(FrameId frameId, const FrameDescriptor& frame) {
    Vec<DebugVariable> result;
    Stack& stack = frame.state->getStack();
    const CallInfo& call = frame.state->getCallStack()[frame.callInfoIndex];
    if (call.func >= stack.size() || !stack[call.func].isFunction()) {
        return std::unexpected(inspectorError(DebugErrorCode::StaleReference, "frame function is unavailable"));
    }
    Function* function = stack[call.func].asFunction();
    Proto* proto = function->getProto();
    if (proto == nullptr) {
        return result;
    }

    HashMap<Str, usize> duplicateNames;
    for (usize index = 0; index < proto->getLocVarCount(); ++index) {
        const LocVar& local = proto->getLocVar(index);
        if (local.varname == nullptr || local.startpc < 0 || local.endpc <= local.startpc ||
            static_cast<usize>(local.endpc) > proto->getInstructionCount() || local.reg < 0 ||
            frame.pc < static_cast<usize>(local.startpc) || frame.pc >= static_cast<usize>(local.endpc)) {
            continue;
        }
        const usize slot = call.base + static_cast<usize>(local.reg);
        if (slot >= stack.size() || slot >= call.top) {
            continue;
        }

        Str name(local.varname->view());
        const usize duplicate = ++duplicateNames[name];
        if (duplicate > 1) {
            name += " (" + std::to_string(duplicate) + ")";
        }
        DebugResult<DebugVariable> variable = makeVariable(std::move(name), stack[slot], frameId);
        if (!variable) {
            return std::unexpected(variable.error());
        }
        result.push_back(std::move(*variable));
    }
    return result;
}

DebugResult<Vec<DebugVariable>> StackInspector::upvalueVariables(FrameId frameId, const FrameDescriptor& frame) {
    Vec<DebugVariable> result;
    Stack& stack = frame.state->getStack();
    const CallInfo& call = frame.state->getCallStack()[frame.callInfoIndex];
    if (call.func >= stack.size() || !stack[call.func].isFunction()) {
        return std::unexpected(inspectorError(DebugErrorCode::StaleReference, "frame function is unavailable"));
    }
    Function* function = stack[call.func].asFunction();
    Proto* proto = function->getProto();
    for (usize index = 0; index < function->getUpvalueCount(); ++index) {
        Upvalue* upvalue = function->getUpvalue(index);
        if (upvalue == nullptr) {
            continue;
        }
        Str name = "(upvalue " + std::to_string(index + 1) + ")";
        if (proto != nullptr && index < proto->getUpvalueNameCount() && proto->getUpvalueName(index) != nullptr) {
            name = proto->getUpvalueName(index)->view();
        }
        DebugResult<DebugVariable> variable = makeVariable(std::move(name), upvalue->getValue(stack), frameId);
        if (!variable) {
            return std::unexpected(variable.error());
        }
        result.push_back(std::move(*variable));
    }
    return result;
}

DebugResult<Vec<DebugVariable>> StackInspector::tableVariables(Table& table, usize start, usize count,
                                                               DebugVariableFilter filter) {
    Vec<DebugVariable> result;
    const usize pageSize = std::min(count == 0 ? limits_.maxVariablePageSize : count, limits_.maxVariablePageSize);
    result.reserve(std::min(pageSize, table.getTotalSize()));

    Value key;
    Value nextKey;
    Value nextValue;
    usize index = 0;
    while (table.next(key, nextKey, nextValue)) {
        bool indexed = false;
        if (nextKey.isNumber()) {
            const LuaNumber number = nextKey.asNumber();
            if (number >= 1 && number <= static_cast<LuaNumber>(table.getArraySize())) {
                const usize integer = static_cast<usize>(number);
                indexed = static_cast<LuaNumber>(integer) == number;
            }
        }
        const bool included = filter == DebugVariableFilter::All ||
                              (filter == DebugVariableFilter::Indexed && indexed) ||
                              (filter == DebugVariableFilter::Named && !indexed);
        if (!included) {
            key = nextKey;
            continue;
        }
        if (index >= start && result.size() < pageSize) {
            DebugResult<DebugVariable> variable = makeVariable(formatTableKey(nextKey), nextValue);
            if (!variable) {
                return std::unexpected(variable.error());
            }
            result.push_back(std::move(*variable));
        }
        ++index;
        if (result.size() >= pageSize) {
            break;
        }
        key = nextKey;
    }
    return result;
}

DebugResult<Vec<DebugVariable>> StackInspector::tableOverview(const VariableNode& node) {
    if (node.table == nullptr) {
        return std::unexpected(inspectorError(DebugErrorCode::InvalidReference, "table handle is empty"));
    }
    Table& table = *node.table;
    Vec<DebugVariable> result;
    result.reserve(3);
    if (table.getArraySize() != 0) {
        auto reference = tableSectionReference(table, node.frame, VariableNodeKind::TableArray);
        if (!reference) {
            return std::unexpected(reference.error());
        }
        DebugVariable array;
        array.name = "[array]";
        array.value = "array (" + std::to_string(table.getArraySize()) + " slots)";
        array.type = "table-array";
        array.variablesReference = *reference;
        array.indexedVariables = table.getArraySize();
        result.push_back(std::move(array));
    }
    if (table.getHashSize() != 0) {
        auto reference = tableSectionReference(table, node.frame, VariableNodeKind::TableHash);
        if (!reference) {
            return std::unexpected(reference.error());
        }
        DebugVariable hash;
        hash.name = "[hash]";
        hash.value = "hash (" + std::to_string(table.getHashSize()) + " entries)";
        hash.type = "table-hash";
        hash.variablesReference = *reference;
        hash.namedVariables = table.getHashSize();
        result.push_back(std::move(hash));
    }
    if (Table* metatable = table.getMetatable()) {
        auto variable = makeVariable("[metatable]", Value(metatable), node.frame);
        if (!variable) {
            return std::unexpected(variable.error());
        }
        variable->type = "metatable";
        result.push_back(std::move(*variable));
    }
    return result;
}

DebugResult<VariableReference> StackInspector::tableSectionReference(Table& table, FrameId originFrame,
                                                                     VariableNodeKind kind) {
    HashMap<const Table*, VariableReference>* references = nullptr;
    if (kind == VariableNodeKind::TableArray) {
        references = &tableArrayReferences_;
    } else if (kind == VariableNodeKind::TableHash) {
        references = &tableHashReferences_;
    } else {
        return std::unexpected(inspectorError(DebugErrorCode::Unsupported, "invalid table section kind"));
    }
    if (const auto found = references->find(&table); found != references->end()) {
        return found->second;
    }
    auto reference = addNode(VariableNode{kind, originFrame, &table});
    if (reference) {
        references->emplace(&table, *reference);
    }
    return reference;
}

DebugResult<DebugVariable> StackInspector::makeVariable(Str name, const Value& value, FrameId originFrame) {
    DebugVariable variable;
    variable.name = std::move(name);
    variable.value = formatValue(value);
    variable.type = valueTypeName(value);
    if (value.isTable() && value.asTable() != nullptr) {
        DebugResult<VariableReference> reference = tableReference(*value.asTable(), originFrame);
        if (!reference) {
            return std::unexpected(reference.error());
        }
        variable.variablesReference = *reference;
        variable.indexedVariables = value.asTable()->getArraySize();
        variable.namedVariables = value.asTable()->getHashSize();
        variable.value = "table#" + std::to_string(reference->value()) + " (" +
                         std::to_string(value.asTable()->getTotalSize()) + " items)" +
                         weakTableSummary(*value.asTable());
    }
    return variable;
}

Str StackInspector::weakTableSummary(const Table& table) const {
    if (state_ == nullptr || table.getMetatable() == nullptr) {
        return {};
    }
    GCString* modeKey = state_->getGlobalState().getStringPool().find("__mode");
    if (modeKey == nullptr) {
        return {};
    }
    const Value mode = table.getMetatable()->get(Value(modeKey));
    if (!mode.isString() || mode.asString() == nullptr) {
        return {};
    }
    const StrView flags = mode.asString()->view();
    const bool weakKeys = flags.find('k') != StrView::npos;
    const bool weakValues = flags.find('v') != StrView::npos;
    if (!weakKeys && !weakValues) {
        return {};
    }
    if (weakKeys && weakValues) {
        return " [weak keys+values]";
    }
    return weakKeys ? Str(" [weak keys]") : Str(" [weak values]");
}

DebugResult<VariableReference> StackInspector::tableReference(Table& table, FrameId originFrame) {
    if (const auto found = tableReferences_.find(&table); found != tableReferences_.end()) {
        return found->second;
    }
    DebugResult<VariableReference> reference =
        addNode(VariableNode{VariableNodeKind::TableOverview, originFrame, &table});
    if (reference) {
        tableReferences_.emplace(&table, *reference);
    }
    return reference;
}

Str StackInspector::formatValue(const Value& value) const {
    if (value.isNil()) {
        return "nil";
    }
    if (value.isBoolean()) {
        return value.asBoolean() ? "true" : "false";
    }
    if (value.isNumber()) {
        std::ostringstream stream;
        stream << std::setprecision(14) << value.asNumber();
        return stream.str();
    }
    if (value.isString()) {
        return formatString(value.asString()->view());
    }
    if (value.isTable()) {
        return "table (" + std::to_string(value.asTable() == nullptr ? 0 : value.asTable()->getTotalSize()) + " items)";
    }
    if (value.isFunction()) {
        return value.asFunction() != nullptr && value.asFunction()->isCFunction() ? "C function" : "function";
    }
    if (value.isUserdata()) {
        return "userdata (" + std::to_string(value.asUserdata() == nullptr ? 0 : value.asUserdata()->getDataSize()) +
               " bytes)";
    }
    if (value.isThread()) {
        if (value.asThread() == nullptr) {
            return "thread";
        }
        switch (value.asThread()->getCoroutineStatus()) {
        case CoroutineStatus::Suspended:
            return "thread (suspended)";
        case CoroutineStatus::Running:
            return "thread (running)";
        case CoroutineStatus::Normal:
            return "thread (normal)";
        case CoroutineStatus::Dead:
            return "thread (dead)";
        }
    }
    if (value.isLightUserdata()) {
        return "lightuserdata";
    }
    return "value";
}

Str StackInspector::formatString(StrView text) const {
    Str escaped = "\"";
    const usize shown = std::min(text.size(), limits_.maxStringLength);
    for (usize index = 0; index < shown; ++index) {
        switch (text[index]) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(text[index]);
            break;
        }
    }
    if (shown < text.size()) {
        escaped += "…";
    }
    escaped += '"';
    return escaped;
}

Str StackInspector::valueTypeName(const Value& value) const {
    switch (value.getType()) {
    case ValueType::Nil:
        return "nil";
    case ValueType::Boolean:
        return "boolean";
    case ValueType::LightUserdata:
        return "lightuserdata";
    case ValueType::Number:
        return "number";
    case ValueType::String:
        return "string";
    case ValueType::Table:
        return "table";
    case ValueType::Function:
        return "function";
    case ValueType::Userdata:
        return "userdata";
    case ValueType::Thread:
        return "thread";
    }
    return "value";
}

Str StackInspector::formatTableKey(const Value& key) const {
    if (key.isString()) {
        return Str(key.asString()->view());
    }
    return "[" + formatValue(key) + "]";
}

} // namespace Lua::Debugger
