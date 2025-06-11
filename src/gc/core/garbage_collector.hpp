#pragma once

#include "../../common/types.hpp"
#include "gc_object.hpp"

namespace Lua {
    // Forward declarations
    class State;
    
    /**
     * @brief Main garbage collector class
     * 
     * This class implements the tri-color mark-and-sweep garbage collection
     * algorithm for the Lua interpreter.
     */
    class GarbageCollector {
    private:
        State* luaState;
        
    public:
        explicit GarbageCollector(State* state) : luaState(state) {}
        
        /**
         * @brief Mark an object as reachable
         * @param obj Pointer to the object to mark
         */
        void markObject(GCObject* obj);
        
        /**
         * @brief Perform a full garbage collection cycle
         */
        void collectGarbage();
        
        /**
         * @brief Check if garbage collection should be triggered
         * @return true if GC should run
         */
        bool shouldCollect() const;
    };
}