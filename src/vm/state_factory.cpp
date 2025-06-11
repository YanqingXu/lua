#include "state_factory.hpp"
#include "../gc/memory/allocator.hpp"

namespace Lua {
    GCRef<State> make_gc_state() {
        extern GCAllocator* g_gcAllocator;
        if (g_gcAllocator) {
            State* obj = g_gcAllocator->allocateObject<State>(GCObjectType::State);
            return GCRef<State>(obj);
        }
        
        // Fallback to direct allocation
        State* obj = new State();
        return GCRef<State>(obj);
    }
    
    GCRef<State> make_gc_state(GCAllocator& allocator) {
        State* obj = allocator.allocateObject<State>(GCObjectType::State);
        return GCRef<State>(obj);
    }
}