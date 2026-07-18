#pragma once

/**
 * @file runtime_services.hpp
 * @brief Explicit bundle of runtime-wide services used by compiler and VM entry points.
 */

#include "runtime/lua_allocator.hpp"
#include "vm/state/global_state.hpp"

#include <exception>

namespace Lua {

namespace VM {
class DispatchStrategy;
}

class EngineContext;

/**
 * @brief Runtime services passed across compiler/VM boundaries.
 *
 * This is intentionally a thin compatibility layer over the current GlobalState-backed
 * runtime. It makes service dependencies explicit at call sites while preserving the
 * existing GlobalState/StringPool/GarbageCollector ownership model.
 */
struct RuntimeServices {
    GlobalState& globalState;
    StringPool& strings;
    GarbageCollector& gc;
    VM::DispatchStrategy* dispatchStrategy;

    explicit RuntimeServices(GlobalState& global, VM::DispatchStrategy* dispatch = nullptr)
        : globalState(global), strings((global.requireOwnerThread(), global.getStringPool())), gc(global.getGC()),
          dispatchStrategy(dispatch) {}

    RuntimeServices(GlobalState& global, StringPool& stringPool, GarbageCollector& collector,
                    VM::DispatchStrategy* dispatch = nullptr)
        : globalState(global), strings((global.requireOwnerThread(), stringPool)), gc(collector),
          dispatchStrategy(dispatch) {}

    static RuntimeServices fromSingletons() {
        return RuntimeServices(GlobalState::getInstance());
    }
};

/**
 * @brief Owning runtime context for isolated Lua states.
 *
 * This is the next step beyond RuntimeServices' non-owning compatibility
 * bundle: each EngineContext owns its string pool and GlobalState, which in
 * turn owns the collector, registry, primitive metatables, reserved strings,
 * and current-thread bookkeeping.
 */
class EngineContext {
public:
    explicit EngineContext(LuaAllocatorFunction allocator = nullptr, void* allocatorUserData = nullptr)
        : allocator_(allocator, allocatorUserData), strings_(&allocator_), globalState_(strings_, &allocator_) {}

    EngineContext(const EngineContext&) = delete;
    EngineContext& operator=(const EngineContext&) = delete;
    EngineContext(EngineContext&&) = delete;
    EngineContext& operator=(EngineContext&&) = delete;

    ~EngineContext() noexcept {
        if (!globalState_.isOwnerThread()) {
            std::terminate();
        }
    }

    [[nodiscard]] RuntimeServices services(VM::DispatchStrategy* dispatch = nullptr) {
        globalState_.requireOwnerThread();
        return RuntimeServices(globalState_, strings_, globalState_.getGC(), dispatch);
    }

    [[nodiscard]] GlobalState& globalState() {
        globalState_.requireOwnerThread();
        return globalState_;
    }

    [[nodiscard]] StringPool& strings() {
        globalState_.requireOwnerThread();
        return strings_;
    }

    [[nodiscard]] GarbageCollector& gc() {
        globalState_.requireOwnerThread();
        return globalState_.getGC();
    }

    [[nodiscard]] NativeModuleRegistry& nativeModules() {
        globalState_.requireOwnerThread();
        return globalState_.getNativeModules();
    }

    [[nodiscard]] const NativeModuleRegistry& nativeModules() const {
        globalState_.requireOwnerThread();
        return globalState_.getNativeModules();
    }

    [[nodiscard]] LuaAllocator& allocator() {
        globalState_.requireOwnerThread();
        return allocator_;
    }

    [[nodiscard]] const LuaAllocator& allocator() const {
        globalState_.requireOwnerThread();
        return allocator_;
    }

    /**
     * @brief Owner-thread access to runtime-wide execution limits.
     */
    [[nodiscard]] ExecutionPolicy& executionPolicy() {
        globalState_.requireOwnerThread();
        return globalState_.getExecutionPolicy();
    }

    [[nodiscard]] const ExecutionPolicy& executionPolicy() const {
        globalState_.requireOwnerThread();
        return globalState_.getExecutionPolicy();
    }

    /**
     * @brief Owner-thread configuration for library and host-resource access.
     */
    [[nodiscard]] SandboxPolicy& sandboxPolicy() {
        globalState_.requireOwnerThread();
        return globalState_.getSandboxPolicy();
    }

    [[nodiscard]] const SandboxPolicy& sandboxPolicy() const {
        globalState_.requireOwnerThread();
        return globalState_.getSandboxPolicy();
    }

    [[nodiscard]] RuntimeRandom& random() {
        globalState_.requireOwnerThread();
        return globalState_.getRandom();
    }

    [[nodiscard]] const RuntimeRandom& random() const {
        globalState_.requireOwnerThread();
        return globalState_.getRandom();
    }

    [[nodiscard]] ResourcePolicy& resourcePolicy() {
        globalState_.requireOwnerThread();
        return globalState_.getResourcePolicy();
    }

    [[nodiscard]] const ResourcePolicy& resourcePolicy() const {
        globalState_.requireOwnerThread();
        return globalState_.getResourcePolicy();
    }

    [[nodiscard]] CompilationPolicy& compilationPolicy() {
        globalState_.requireOwnerThread();
        return globalState_.getCompilationPolicy();
    }

    [[nodiscard]] const CompilationPolicy& compilationPolicy() const {
        globalState_.requireOwnerThread();
        return globalState_.getCompilationPolicy();
    }

    [[nodiscard]] TraceRuntime& trace() {
        globalState_.requireOwnerThread();
        return globalState_.getTraceRuntime();
    }

    [[nodiscard]] const TraceRuntime& trace() const {
        globalState_.requireOwnerThread();
        return globalState_.getTraceRuntime();
    }

    /**
     * @brief Create a teardown-safe handle whose only cross-thread action is cancel.
     */
    [[nodiscard]] ExecutionCancellationHandle cancellationHandle() {
        return executionPolicy().cancellationHandle();
    }

private:
    LuaAllocator allocator_;
    StringPool strings_;
    GlobalState globalState_;
};

} // namespace Lua
