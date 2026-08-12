/**
 * @file breakpoint_manager.cpp
 * @brief Breakpoint binding and immutable instruction lookup implementation.
 */

#include "debugger/breakpoint_manager.hpp"

#include "core/function.hpp"

#include <algorithm>
#include <bit>
#include <charconv>

namespace Lua::Debugger {

namespace {

constexpr usize kMaxBreakpointExpressionBytes = 4096;
constexpr usize kMaxLogMessageBytes = 4096;

struct ParsedBehavior {
    Ptr<const BreakpointBehavior> behavior;
    Opt<Str> error;
};

ParsedBehavior parseBehavior(StrView condition, StrView hitCondition, StrView logMessage) {
    Ptr<BreakpointBehavior> behavior = makePtr<BreakpointBehavior>();
    if (condition.size() > kMaxBreakpointExpressionBytes) {
        return {behavior, Str("condition exceeds the configured byte limit")};
    }
    if (logMessage.size() > kMaxLogMessageBytes) {
        return {behavior, Str("log message exceeds the configured byte limit")};
    }
    behavior->condition = Str(condition);
    behavior->logMessage = Str(logMessage);
    if (!hitCondition.empty()) {
        u64 target = 0;
        const char* first = hitCondition.data();
        const char* last = first + hitCondition.size();
        const auto parsed = std::from_chars(first, last, target, 10);
        if (parsed.ec != std::errc{} || parsed.ptr != last || target == 0) {
            return {behavior, Str("hitCondition must be a positive decimal integer")};
        }
        behavior->hitTarget = target;
    }
    return {std::move(behavior), {}};
}

Str protoIdentity(const Proto& proto) {
    const NormalizedSource source =
        normalizeSourceName(proto.getSource() == nullptr ? StrView{} : proto.getSource()->view());
    return source.displayName + ":" + std::to_string(std::max(proto.getLineDefined(), 0));
}

bool functionMatches(const Proto& proto, StrView requested) {
    return (proto.getDebugName() != nullptr && proto.getDebugName()->view() == requested) ||
           protoIdentity(proto) == requested;
}

constexpr u64 kFnvOffset = 14695981039346656037ULL;
constexpr u64 kFnvPrime = 1099511628211ULL;

void hashBytes(u64& hash, const void* data, usize size) {
    const auto* bytes = static_cast<const u8*>(data);
    for (usize index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= kFnvPrime;
    }
}

template <typename T> void hashScalar(u64& hash, const T& value) {
    hashBytes(hash, &value, sizeof(value));
}

void hashText(u64& hash, StrView text) {
    hashScalar(hash, text.size());
    hashBytes(hash, text.data(), text.size());
}

u64 protoContentIdentity(const Proto& proto) {
    u64 hash = kFnvOffset;
    hashScalar(hash, proto.getLineDefined());
    hashScalar(hash, proto.getLastLineDefined());
    hashScalar(hash, proto.getNumParams());
    hashScalar(hash, proto.getVarargFlags());
    if (proto.getDebugName() != nullptr) {
        hashText(hash, proto.getDebugName()->view());
    }
    for (const Instruction instruction : proto.getInstructionSpan()) {
        hashScalar(hash, instruction);
    }
    for (const i32 line : proto.getLineInfo()) {
        hashScalar(hash, line);
    }
    for (usize index = 0; index < proto.getConstantCount(); ++index) {
        const Value value = proto.getConstant(index);
        hashScalar(hash, value.getType());
        if (value.isBoolean()) {
            hashScalar(hash, value.asBoolean());
        } else if (value.isNumber()) {
            const u64 bits = std::bit_cast<u64>(value.asNumber());
            hashScalar(hash, bits);
        } else if (value.isString() && value.asString() != nullptr) {
            hashText(hash, value.asString()->view());
        }
    }
    return hash;
}

} // namespace

BreakpointManager::BreakpointManager() : emptySnapshot_(makePtr<const LookupSnapshot>()), snapshot_(emptySnapshot_) {}

usize BreakpointManager::InstructionKeyHash::operator()(const InstructionKey& key) const noexcept {
    const usize protoHash = std::hash<const Proto*>{}(key.proto);
    const usize pcHash = std::hash<usize>{}(key.pc);
    return protoHash ^ (pcHash + static_cast<usize>(0x9e3779b9U) + (protoHash << 6U) + (protoHash >> 2U));
}

SourceId BreakpointManager::registerSourceName(StrView rawSource) {
    std::lock_guard lock(mutex_);
    return sources_.registerSource(rawSource);
}

SourceId BreakpointManager::registerFilePath(StrView path) {
    std::lock_guard lock(mutex_);
    return sources_.registerFilePath(path);
}

Opt<RegisteredSource> BreakpointManager::source(SourceId id) const {
    std::lock_guard lock(mutex_);
    const RegisteredSource* registered = sources_.lookup(id);
    return registered == nullptr ? std::nullopt : Opt<RegisteredSource>{*registered};
}

Opt<u64> BreakpointManager::sourceContentIdentity(SourceId id) const {
    std::lock_guard lock(mutex_);
    const auto found = sourceLoads_.find(id.value());
    return found == sourceLoads_.end() ? Opt<u64>{} : Opt<u64>{found->second.contentIdentity};
}

DebugResult<Vec<BreakpointBinding>> BreakpointManager::setBreakpoints(SourceId sourceId,
                                                                      std::span<const SourceBreakpoint> requested) {
    std::lock_guard lock(mutex_);
    if (sources_.lookup(sourceId) == nullptr) {
        return std::unexpected(DebugError{DebugErrorCode::InvalidReference, "unknown source ID"});
    }

    Vec<RequestedBreakpoint> replacements;
    replacements.reserve(requested.size());
    for (const SourceBreakpoint& sourceBreakpoint : requested) {
        RequestedBreakpoint replacement;
        replacement.binding.id = BreakpointId{nextBreakpointId_++};
        replacement.binding.sourceId = sourceId;
        replacement.binding.requestedLine = sourceBreakpoint.line;
        ParsedBehavior parsed =
            parseBehavior(sourceBreakpoint.condition, sourceBreakpoint.hitCondition, sourceBreakpoint.logMessage);
        replacement.behavior = std::move(parsed.behavior);
        replacement.validationError = std::move(parsed.error);
        bindLocked(replacement);
        replacements.push_back(std::move(replacement));
    }
    requestedBySource_[sourceId.value()] = std::move(replacements);
    rebuildSnapshotLocked();

    Vec<BreakpointBinding> result;
    result.reserve(requestedBySource_[sourceId.value()].size());
    for (const RequestedBreakpoint& breakpoint : requestedBySource_[sourceId.value()]) {
        result.push_back(breakpoint.binding);
    }
    return result;
}

DebugResult<Vec<BreakpointBinding>>
BreakpointManager::setFunctionBreakpoints(std::span<const FunctionBreakpoint> requested) {
    std::lock_guard lock(mutex_);
    Vec<RequestedBreakpoint> replacements;
    replacements.reserve(requested.size());
    for (const FunctionBreakpoint& functionBreakpoint : requested) {
        RequestedBreakpoint replacement;
        replacement.binding.id = BreakpointId{nextBreakpointId_++};
        replacement.binding.functionName = functionBreakpoint.name;
        ParsedBehavior parsed = parseBehavior(functionBreakpoint.condition, functionBreakpoint.hitCondition, {});
        replacement.behavior = std::move(parsed.behavior);
        replacement.validationError = std::move(parsed.error);
        if (functionBreakpoint.name.empty() || functionBreakpoint.name.size() > kMaxBreakpointExpressionBytes) {
            replacement.validationError = "function breakpoint name is empty or exceeds the configured byte limit";
        }
        bindFunctionLocked(replacement);
        replacements.push_back(std::move(replacement));
    }
    requestedFunctions_ = std::move(replacements);
    rebuildSnapshotLocked();

    Vec<BreakpointBinding> result;
    result.reserve(requestedFunctions_.size());
    for (const RequestedBreakpoint& breakpoint : requestedFunctions_) {
        result.push_back(breakpoint.binding);
    }
    return result;
}

Opt<BreakpointActivation> BreakpointManager::recordHit(BreakpointId id) noexcept {
    std::lock_guard lock(mutex_);
    const auto activate = [&](RequestedBreakpoint& breakpoint) -> Opt<BreakpointActivation> {
        if (breakpoint.binding.id != id) {
            return {};
        }
        ++breakpoint.hitCount;
        const u64 target = breakpoint.behavior == nullptr ? 0 : breakpoint.behavior->hitTarget;
        return BreakpointActivation{breakpoint.behavior, breakpoint.hitCount,
                                    target == 0 || breakpoint.hitCount == target};
    };
    for (auto& [source, breakpoints] : requestedBySource_) {
        (void)source;
        for (RequestedBreakpoint& breakpoint : breakpoints) {
            if (Opt<BreakpointActivation> result = activate(breakpoint)) {
                return result;
            }
        }
    }
    for (RequestedBreakpoint& breakpoint : requestedFunctions_) {
        if (Opt<BreakpointActivation> result = activate(breakpoint)) {
            return result;
        }
    }
    return {};
}

void BreakpointManager::clearBreakpoints() noexcept {
    std::lock_guard lock(mutex_);
    requestedBySource_.clear();
    requestedFunctions_.clear();
    hasBreakpoints_.store(false, std::memory_order_release);
    storeSnapshot(emptySnapshot_);
}

Vec<BreakpointBinding> BreakpointManager::registerProto(const Proto& root) {
    std::lock_guard lock(mutex_);
    if (registeredProtos_.contains(&root)) {
        return {};
    }

    DebugInfoIndex index(root);
    HashMap<u64, Vec<DebugCodeLocation>> incomingLocations;
    HashMap<u64, SourceLoad> incomingLoads;
    for (const ProtoDebugInfoStatus& status : index.protoStatuses()) {
        if (status.proto == nullptr) {
            continue;
        }
        registeredProtos_.insert(status.proto);
        if (status.source.valid()) {
            const SourceId sourceId = sources_.registerNormalizedSource(status.source);
            SourceLoad& load = incomingLoads[sourceId.value()];
            if (load.protos.empty()) {
                load.contentIdentity = kFnvOffset;
            }
            const u64 protoIdentity = protoContentIdentity(*status.proto);
            hashScalar(load.contentIdentity, protoIdentity);
            load.protos.insert(status.proto);
        }
    }

    for (const DebugCodeLocation& location : index.allLocations()) {
        const SourceId sourceId = sources_.registerNormalizedSource(location.source);
        if (!sourceId.valid()) {
            continue;
        }
        incomingLocations[sourceId.value()].push_back(location);
        incomingLoads[sourceId.value()].protos.insert(location.proto);
    }

    HashSet<u64> reloadedSources;
    for (auto& [sourceValue, load] : incomingLoads) {
        const auto previous = sourceLoads_.find(sourceValue);
        if (previous != sourceLoads_.end()) {
            reloadedSources.insert(sourceValue);
            for (const Proto* proto : previous->second.protos) {
                registeredProtos_.erase(proto);
            }
        }
        for (const Proto* proto : load.protos) {
            registeredProtos_.insert(proto);
        }
        sourceLoads_[sourceValue] = std::move(load);
        const auto locations = incomingLocations.find(sourceValue);
        locationsBySource_[sourceValue] =
            locations == incomingLocations.end() ? Vec<DebugCodeLocation>{} : std::move(locations->second);
    }

    Vec<BreakpointBinding> changed;
    for (const auto& [sourceValue, load] : incomingLoads) {
        (void)load;
        const auto requested = requestedBySource_.find(sourceValue);
        if (requested == requestedBySource_.end()) {
            continue;
        }
        for (RequestedBreakpoint& breakpoint : requested->second) {
            const BreakpointBinding previous = breakpoint.binding;
            bindLocked(breakpoint);
            if (reloadedSources.contains(sourceValue) || previous.verified != breakpoint.binding.verified ||
                previous.line != breakpoint.binding.line || previous.message != breakpoint.binding.message) {
                changed.push_back(breakpoint.binding);
            }
        }
    }
    for (RequestedBreakpoint& breakpoint : requestedFunctions_) {
        const BreakpointBinding previous = breakpoint.binding;
        bindFunctionLocked(breakpoint);
        if (!reloadedSources.empty() || previous.verified != breakpoint.binding.verified ||
            previous.line != breakpoint.binding.line || previous.sourceId != breakpoint.binding.sourceId ||
            previous.message != breakpoint.binding.message) {
            changed.push_back(breakpoint.binding);
        }
    }
    rebuildSnapshotLocked();
    return changed;
}

Ptr<const BreakpointHitList> BreakpointManager::match(const Proto& proto, usize pc) const noexcept {
    const Ptr<const LookupSnapshot> snapshot = loadSnapshot();
    const auto found = snapshot->hits.find(InstructionKey{&proto, pc});
    if (found == snapshot->hits.end()) {
        return {};
    }
    return found->second;
}

void BreakpointManager::bindLocked(RequestedBreakpoint& breakpoint) {
    breakpoint.locations.clear();
    BreakpointBinding& binding = breakpoint.binding;
    binding.line = binding.requestedLine;
    binding.verified = false;

    if (breakpoint.validationError) {
        binding.message = *breakpoint.validationError;
        return;
    }

    if (binding.requestedLine <= 0) {
        binding.message = "line must be a positive 1-based value";
        return;
    }

    const auto sourceLocations = locationsBySource_.find(binding.sourceId.value());
    if (sourceLocations == locationsBySource_.end() || sourceLocations->second.empty()) {
        binding.message = "pending: source has not been loaded or has no line information";
        return;
    }

    Opt<i32> resolvedLine;
    for (const DebugCodeLocation& location : sourceLocations->second) {
        if (location.line >= binding.requestedLine && (!resolvedLine || location.line < *resolvedLine)) {
            resolvedLine = location.line;
        }
    }
    if (!resolvedLine) {
        binding.message = "no executable line at or after the requested line";
        return;
    }

    for (const DebugCodeLocation& location : sourceLocations->second) {
        if (location.line == *resolvedLine) {
            breakpoint.locations.push_back(location);
        }
    }
    binding.line = *resolvedLine;
    binding.verified = !breakpoint.locations.empty();
    binding.message = binding.verified ? Str{} : Str("line information is unavailable");
}

void BreakpointManager::bindFunctionLocked(RequestedBreakpoint& breakpoint) {
    breakpoint.locations.clear();
    BreakpointBinding& binding = breakpoint.binding;
    binding.sourceId = {};
    binding.line = 0;
    binding.verified = false;
    if (breakpoint.validationError) {
        binding.message = *breakpoint.validationError;
        return;
    }
    if (!binding.functionName || binding.functionName->empty()) {
        binding.message = "function breakpoint name is empty";
        return;
    }
    for (const Proto* proto : registeredProtos_) {
        if (proto == nullptr || proto->getInstructionCount() == 0 || !functionMatches(*proto, *binding.functionName)) {
            continue;
        }
        const NormalizedSource source =
            normalizeSourceName(proto->getSource() == nullptr ? StrView{} : proto->getSource()->view());
        const SourceId sourceId = sources_.registerNormalizedSource(source);
        const i32 line = proto->getLine(0) > 0 ? proto->getLine(0) : proto->getLineDefined();
        breakpoint.locations.push_back(DebugCodeLocation{proto, 0, line, source});
        if (!binding.sourceId.valid()) {
            binding.sourceId = sourceId;
            binding.line = line;
        }
    }
    binding.verified = !breakpoint.locations.empty();
    binding.message = binding.verified
                          ? Str{}
                          : Str("no loaded function matches; use a compiler name or source:definition-line identity");
}

void BreakpointManager::rebuildSnapshotLocked() {
    Ptr<LookupSnapshot> next = makePtr<LookupSnapshot>();
    std::unordered_map<InstructionKey, BreakpointHitList, InstructionKeyHash> staged;
    for (const auto& [sourceId, breakpoints] : requestedBySource_) {
        (void)sourceId;
        for (const RequestedBreakpoint& breakpoint : breakpoints) {
            if (!breakpoint.binding.verified) {
                continue;
            }
            for (const DebugCodeLocation& location : breakpoint.locations) {
                staged[InstructionKey{location.proto, location.pc}].push_back(BreakpointHit{
                    breakpoint.binding.id, breakpoint.binding.sourceId, breakpoint.binding.line, breakpoint.behavior});
            }
        }
    }
    for (const RequestedBreakpoint& breakpoint : requestedFunctions_) {
        if (!breakpoint.binding.verified) {
            continue;
        }
        for (const DebugCodeLocation& location : breakpoint.locations) {
            staged[InstructionKey{location.proto, location.pc}].push_back(
                BreakpointHit{breakpoint.binding.id, sources_.registerNormalizedSource(location.source), location.line,
                              breakpoint.behavior});
        }
    }
    next->hits.reserve(staged.size());
    for (auto& [instruction, hits] : staged) {
        next->hits.emplace(instruction, makePtr<const BreakpointHitList>(std::move(hits)));
    }
    hasBreakpoints_.store(!next->hits.empty(), std::memory_order_release);
    storeSnapshot(std::move(next));
}

Ptr<const BreakpointManager::LookupSnapshot> BreakpointManager::loadSnapshot() const noexcept {
#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    return snapshot_.load(std::memory_order_acquire);
#else
    return std::atomic_load_explicit(&snapshot_, std::memory_order_acquire);
#endif
}

void BreakpointManager::storeSnapshot(Ptr<const LookupSnapshot> snapshot) noexcept {
#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    snapshot_.store(std::move(snapshot), std::memory_order_release);
#else
    std::atomic_store_explicit(&snapshot_, std::move(snapshot), std::memory_order_release);
#endif
}

} // namespace Lua::Debugger
