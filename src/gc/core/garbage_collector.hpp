#pragma once

#include "../../common/types.hpp"
#include "gc_object.hpp"
#include "../algorithms/gc_marker.hpp"
#include "../algorithms/gc_sweeper.hpp"
#include "../utils/gc_types.hpp"
#include "string_pool.hpp"

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
        
        // GC algorithm components
        GCMarker marker;
        GCSweeper sweeper;
        
        // GC state and configuration
        GCState gcState;
        GCColor currentWhite;
        GCStats stats;
        GCConfig config;
        
        // Object management
        GCObject* allObjectsHead;
        usize gcThreshold;
        usize totalAllocated;
        
    public:
        explicit GarbageCollector(State* state) 
            : luaState(state)
            , marker()
            , sweeper(1024)  // Process up to 1024 objects per incremental step
            , gcState(GCState::Pause)
            , currentWhite(GCColor::White0)
            , allObjectsHead(nullptr)
            , gcThreshold(1024 * 1024)  // 1MB default threshold
            , totalAllocated(0) {
            stats.reset();
        }
        
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
        
        /**
         * @brief Register a new object with the GC
         * @param obj Object to register
         */
        void registerObject(GCObject* obj);
        
        /**
         * @brief Update memory allocation statistics
         * @param delta Change in allocated memory (can be negative for deallocation)
         */
        void updateAllocatedMemory(isize delta);
        
        /**
         * @brief Get current GC statistics
         * @return Reference to GC statistics
         */
        const GCStats& getStats() const { return stats; }
        
        /**
         * @brief Get current GC configuration
         * @return Reference to GC configuration
         */
        const GCConfig& getConfig() const { return config; }
        
        /**
         * @brief Update GC configuration
         * @param newConfig New configuration to apply
         */
        void setConfig(const GCConfig& newConfig) { config = newConfig; }
        
    private:
        /**
         * @brief Collect root objects for marking phase
         * @return Vector of root objects
         */
        std::vector<GCObject*> collectRootObjects();
        
        /**
         * @brief Perform the mark phase of GC
         */
        void markPhase();
        
        /**
         * @brief Perform the sweep phase of GC
         */
        void sweepPhase();
        
        /**
         * @brief Flip white colors for next collection cycle
         */
        void flipWhiteColors();
    };
}