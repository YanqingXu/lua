#pragma once

/**
 * @file breakpoint_manager.hpp
 * @brief Source-oriented breakpoint replacement, binding, and hot-path lookup.
 */

#include "debugger/debug_types.hpp"
#include "debugger/source_registry.hpp"

#include <atomic>
#include <mutex>
#include <span>

namespace Lua {
class Proto;
}

namespace Lua::Debugger {

struct SourceBreakpoint {
    i32 line = 0;
};

struct BreakpointBinding {
    BreakpointId id;
    SourceId sourceId;
    i32 requestedLine = 0;
    i32 line = 0;
    bool verified = false;
    Str message;
};

struct BreakpointHit {
    BreakpointId id;
    SourceId sourceId;
    i32 line = 0;
};

class BreakpointManager {
public:
    BreakpointManager();

    [[nodiscard]] SourceId registerSourceName(StrView rawSource);
    [[nodiscard]] SourceId registerFilePath(StrView path);
    [[nodiscard]] Opt<RegisteredSource> source(SourceId id) const;

    [[nodiscard]] DebugResult<Vec<BreakpointBinding>> setBreakpoints(SourceId sourceId,
                                                                     std::span<const SourceBreakpoint> requested);

    /** Register a complete Proto tree and return pending bindings that changed. */
    [[nodiscard]] Vec<BreakpointBinding> registerProto(const Proto& root);

    /** Allocation-free, formatting-free lookup used immediately before an opcode. */
    [[nodiscard]] Opt<BreakpointHit> match(const Proto& proto, usize pc) const noexcept;

    [[nodiscard]] bool hasBreakpoints() const noexcept {
        return hasBreakpoints_.load(std::memory_order_acquire);
    }

private:
    struct RequestedBreakpoint {
        BreakpointBinding binding;
        Vec<DebugCodeLocation> locations;
    };

    struct InstructionKey {
        const Proto* proto = nullptr;
        usize pc = 0;

        bool operator==(const InstructionKey&) const noexcept = default;
    };

    struct InstructionKeyHash {
        usize operator()(const InstructionKey& key) const noexcept;
    };

    struct LookupSnapshot {
        std::unordered_map<InstructionKey, BreakpointHit, InstructionKeyHash> hits;
    };

    void bindLocked(RequestedBreakpoint& breakpoint);
    void rebuildSnapshotLocked();

    mutable std::mutex mutex_;
    SourceRegistry sources_;
    HashMap<u64, Vec<DebugCodeLocation>> locationsBySource_;
    HashMap<u64, Vec<RequestedBreakpoint>> requestedBySource_;
    HashSet<const Proto*> registeredProtos_;
    std::atomic<Ptr<const LookupSnapshot>> snapshot_;
    std::atomic<bool> hasBreakpoints_ = false;
    u64 nextBreakpointId_ = 1;
};

} // namespace Lua::Debugger
