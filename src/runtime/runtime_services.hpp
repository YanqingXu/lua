#pragma once

/**
 * @file runtime_services.hpp
 * @brief Explicit bundle of runtime-wide services used by compiler and VM entry points.
 */

#include "vm/global_state.hpp"

namespace Lua {

namespace VM {
class DispatchStrategy;
}

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

}  // namespace Lua
