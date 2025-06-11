#pragma once

#include "state.hpp"
#include "../gc/core/gc_ref.hpp"
#include "../gc/memory/allocator.hpp"

namespace Lua {
    /**
     * @brief Factory function to create a GC-managed State object
     * 
     * This function ensures that State objects are properly allocated
     * through the GC allocator and registered as root objects.
     */
    GCRef<State> make_gc_state();
    
    /**
     * @brief Create a State object with a specific GC allocator
     */
    GCRef<State> make_gc_state(GCAllocator& allocator);
}