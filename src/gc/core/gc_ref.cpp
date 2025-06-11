#include "gc_ref.hpp"
#include "../../vm/function.hpp"
#include "../memory/allocator.hpp"

namespace Lua {
    extern GCAllocator* g_gcAllocator;
    
    GCRef<Function> make_gc_function(int functionType) {
        Function::Type type = static_cast<Function::Type>(functionType);
        if (g_gcAllocator) {
            Function* obj = g_gcAllocator->allocateObject<Function>(GCObjectType::Function, type);
            return GCRef<Function>(obj);
        }
        
        // Fallback to direct allocation
        Function* obj = new Function(type);
        return GCRef<Function>(obj);
    }
}