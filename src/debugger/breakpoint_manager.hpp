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
#include <version>

namespace Lua {
class Proto;
}

namespace Lua::Debugger {

struct SourceBreakpoint {
    i32 line = 0;
    Str condition;
    Str hitCondition;
    Str logMessage;
};

struct FunctionBreakpoint {
    Str name;
    Str condition;
    Str hitCondition;
};

struct BreakpointBinding {
    BreakpointId id;
    SourceId sourceId;
    i32 requestedLine = 0;
    i32 line = 0;
    bool verified = false;
    Str message;
    Opt<Str> functionName;
};

struct BreakpointBehavior {
    Str condition;
    u64 hitTarget = 0;
    Str logMessage;
};

struct BreakpointHit {
    BreakpointId id;
    SourceId sourceId;
    i32 line = 0;
    Ptr<const BreakpointBehavior> behavior;
};

using BreakpointHitList = Vec<BreakpointHit>;

struct BreakpointActivation {
    Ptr<const BreakpointBehavior> behavior;
    u64 hitCount = 0;
    bool hitTargetReached = true;
};

class BreakpointManager {
public:
    BreakpointManager();

    [[nodiscard]] SourceId registerSourceName(StrView rawSource);
    [[nodiscard]] SourceId registerFilePath(StrView path);
    [[nodiscard]] Opt<RegisteredSource> source(SourceId id) const;
    [[nodiscard]] Opt<u64> sourceContentIdentity(SourceId id) const;

    [[nodiscard]] DebugResult<Vec<BreakpointBinding>> setBreakpoints(SourceId sourceId,
                                                                     std::span<const SourceBreakpoint> requested);
    [[nodiscard]] DebugResult<Vec<BreakpointBinding>>
    setFunctionBreakpoints(std::span<const FunctionBreakpoint> requested);

    /** Record one logical line/function hit. Count is global per breakpoint and survives Proto rebinding. */
    [[nodiscard]] Opt<BreakpointActivation> recordHit(BreakpointId id) noexcept;

    /** Remove session-owned breakpoints while preserving source and Proto metadata. */
    void clearBreakpoints() noexcept;

    /** Register a complete Proto tree and return pending bindings that changed. */
    [[nodiscard]] Vec<BreakpointBinding> registerProto(const Proto& root);

    /** Allocation-free, formatting-free lookup used immediately before an opcode. */
    [[nodiscard]] Ptr<const BreakpointHitList> match(const Proto& proto, usize pc) const noexcept;

    [[nodiscard]] bool hasBreakpoints() const noexcept {
        return hasBreakpoints_.load(std::memory_order_acquire);
    }

private:
    struct RequestedBreakpoint {
        BreakpointBinding binding;
        Vec<DebugCodeLocation> locations;
        Ptr<const BreakpointBehavior> behavior;
        u64 hitCount = 0;
        Opt<Str> validationError;
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
        std::unordered_map<InstructionKey, Ptr<const BreakpointHitList>, InstructionKeyHash> hits;
    };

    struct SourceLoad {
        u64 contentIdentity = 0;
        HashSet<const Proto*> protos;
    };

    void bindLocked(RequestedBreakpoint& breakpoint);
    void bindFunctionLocked(RequestedBreakpoint& breakpoint);
    void rebuildSnapshotLocked();
    [[nodiscard]] Ptr<const LookupSnapshot> loadSnapshot() const noexcept;
    void storeSnapshot(Ptr<const LookupSnapshot> snapshot) noexcept;

    mutable std::mutex mutex_;
    SourceRegistry sources_;
    HashMap<u64, Vec<DebugCodeLocation>> locationsBySource_;
    HashMap<u64, Vec<RequestedBreakpoint>> requestedBySource_;
    HashMap<u64, SourceLoad> sourceLoads_;
    Vec<RequestedBreakpoint> requestedFunctions_;
    HashSet<const Proto*> registeredProtos_;
    Ptr<const LookupSnapshot> emptySnapshot_;
#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    std::atomic<Ptr<const LookupSnapshot>> snapshot_;
#else
    Ptr<const LookupSnapshot> snapshot_;
#endif
    std::atomic<bool> hasBreakpoints_ = false;
    u64 nextBreakpointId_ = 1;
};

} // namespace Lua::Debugger
