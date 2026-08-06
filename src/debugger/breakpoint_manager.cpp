/**
 * @file breakpoint_manager.cpp
 * @brief Breakpoint binding and immutable instruction lookup implementation.
 */

#include "debugger/breakpoint_manager.hpp"

#include "core/function.hpp"

#include <algorithm>

namespace Lua::Debugger {

BreakpointManager::BreakpointManager() : snapshot_(makePtr<const LookupSnapshot>()) {}

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

Vec<BreakpointBinding> BreakpointManager::registerProto(const Proto& root) {
    std::lock_guard lock(mutex_);
    if (registeredProtos_.contains(&root)) {
        return {};
    }

    DebugInfoIndex index(root);
    for (const ProtoDebugInfoStatus& status : index.protoStatuses()) {
        if (status.proto != nullptr) {
            registeredProtos_.insert(status.proto);
        }
    }

    HashSet<u64> affectedSources;
    for (const DebugCodeLocation& location : index.allLocations()) {
        const SourceId sourceId = sources_.registerNormalizedSource(location.source);
        if (!sourceId.valid()) {
            continue;
        }
        locationsBySource_[sourceId.value()].push_back(location);
        affectedSources.insert(sourceId.value());
    }

    Vec<BreakpointBinding> changed;
    for (const u64 sourceValue : affectedSources) {
        const auto requested = requestedBySource_.find(sourceValue);
        if (requested == requestedBySource_.end()) {
            continue;
        }
        for (RequestedBreakpoint& breakpoint : requested->second) {
            const BreakpointBinding previous = breakpoint.binding;
            bindLocked(breakpoint);
            if (previous.verified != breakpoint.binding.verified || previous.line != breakpoint.binding.line ||
                previous.message != breakpoint.binding.message) {
                changed.push_back(breakpoint.binding);
            }
        }
    }
    rebuildSnapshotLocked();
    return changed;
}

Opt<BreakpointHit> BreakpointManager::match(const Proto& proto, usize pc) const noexcept {
    const Ptr<const LookupSnapshot> snapshot = snapshot_.load(std::memory_order_acquire);
    const auto found = snapshot->hits.find(InstructionKey{&proto, pc});
    if (found == snapshot->hits.end()) {
        return std::nullopt;
    }
    return found->second;
}

void BreakpointManager::bindLocked(RequestedBreakpoint& breakpoint) {
    breakpoint.locations.clear();
    BreakpointBinding& binding = breakpoint.binding;
    binding.line = binding.requestedLine;
    binding.verified = false;

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

void BreakpointManager::rebuildSnapshotLocked() {
    Ptr<LookupSnapshot> next = makePtr<LookupSnapshot>();
    for (const auto& [sourceId, breakpoints] : requestedBySource_) {
        (void)sourceId;
        for (const RequestedBreakpoint& breakpoint : breakpoints) {
            if (!breakpoint.binding.verified) {
                continue;
            }
            for (const DebugCodeLocation& location : breakpoint.locations) {
                next->hits.try_emplace(
                    InstructionKey{location.proto, location.pc},
                    BreakpointHit{breakpoint.binding.id, breakpoint.binding.sourceId, breakpoint.binding.line});
            }
        }
    }
    hasBreakpoints_.store(!next->hits.empty(), std::memory_order_release);
    snapshot_.store(std::move(next), std::memory_order_release);
}

} // namespace Lua::Debugger
