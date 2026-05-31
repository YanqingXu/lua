#pragma once

/**
 * @file runtime_services.hpp
 * @brief Explicit bundle of runtime-wide services used by compiler and VM entry points.
 */

#include "vm/state/global_state.hpp"

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
        : globalState(global)
        , strings(global.getStringPool())
        , gc(global.getGC())
        , dispatchStrategy(dispatch) {}

    RuntimeServices(GlobalState& global, StringPool& stringPool, GarbageCollector& collector,
                    VM::DispatchStrategy* dispatch = nullptr)
        : globalState(global)
        , strings(stringPool)
        , gc(collector)
        , dispatchStrategy(dispatch) {}

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
    EngineContext()
        : strings_()
        , globalState_(strings_) {}

    EngineContext(const EngineContext&) = delete;
    EngineContext& operator=(const EngineContext&) = delete;
    EngineContext(EngineContext&&) = delete;
    EngineContext& operator=(EngineContext&&) = delete;

    [[nodiscard]] RuntimeServices services(VM::DispatchStrategy* dispatch = nullptr) noexcept {
        return RuntimeServices(globalState_, strings_, globalState_.getGC(), dispatch);
    }

    [[nodiscard]] GlobalState& globalState() noexcept {
        return globalState_;
    }

    [[nodiscard]] StringPool& strings() noexcept {
        return strings_;
    }

    [[nodiscard]] GarbageCollector& gc() noexcept {
        return globalState_.getGC();
    }

private:
    StringPool strings_;
    GlobalState globalState_;
};

}  // namespace Lua
