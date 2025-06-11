#pragma once

#include "../core/gc_object.hpp"
#include "../utils/gc_types.hpp"
#include "../../common/types.hpp"
#include <vector>
#include <functional>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class State;
    class StringPool;

    /**
     * @brief Sweep phase implementation for mark-and-sweep garbage collection
     * 
     * This class implements the sweep phase of the tri-color mark-and-sweep
     * garbage collection algorithm. It traverses all allocated objects and
     * frees those that are marked as white (unreachable).
     * 
     * The sweeper works in conjunction with the GCMarker to complete the
     * mark-and-sweep cycle:
     * 1. Marker marks all reachable objects
     * 2. Sweeper frees all unmarked (white) objects
     * 3. Sweeper flips white colors for next cycle
     */
    class GCSweeper {
    public:
        /**
         * @brief Statistics for sweep operation
         */
        struct SweepStats {
            usize objectsSwept = 0;         // Total objects processed
            usize objectsFreed = 0;         // Objects freed
            usize bytesFreed = 0;           // Bytes freed
            usize objectsKept = 0;          // Objects kept alive
            u64 sweepTimeUs = 0;            // Sweep time in microseconds
            usize finalizersRun = 0;        // Number of finalizers executed
            
            void reset() {
                objectsSwept = objectsFreed = bytesFreed = objectsKept = 0;
                sweepTimeUs = finalizersRun = 0;
            }
        };
        
        /**
         * @brief Callback function type for object finalization
         */
        using FinalizerCallback = std::function<void(GCObject*)>;
        
    private:
        // Current white color for this sweep cycle
        GCColor currentWhite;
        
        // Next white color for next cycle
        GCColor nextWhite;
        
        // Statistics for current sweep
        SweepStats stats;
        
        // Finalizer callback
        FinalizerCallback finalizerCallback;
        
        // Objects pending finalization
        std::vector<GCObject*> finalizationQueue;
        
        // Maximum objects to process per incremental step
        usize maxStepSize;
        
        // Current position in object list for incremental sweeping
        GCObject* currentPosition;
        
    public:
        /**
         * @brief Construct a new GCSweeper
         * @param stepSize Maximum objects to process per incremental step
         */
        explicit GCSweeper(usize stepSize = 1024);
        
        /**
         * @brief Destroy the GCSweeper
         */
        ~GCSweeper() = default;
        
        // Non-copyable and non-movable
        GCSweeper(const GCSweeper&) = delete;
        GCSweeper& operator=(const GCSweeper&) = delete;
        GCSweeper(GCSweeper&&) = delete;
        GCSweeper& operator=(GCSweeper&&) = delete;
        
        /**
         * @brief Start a new sweep cycle
         * @param objectList Head of the object list to sweep
         * @param white Current white color for this cycle
         */
        void startSweep(GCObject* objectList, GCColor white);
        
        /**
         * @brief Perform a complete sweep of all objects
         * @param objectList Head of the object list to sweep
         * @param white Current white color for this cycle
         * @return New head of the object list after sweep
         */
        GCObject* sweepAll(GCObject* objectList, GCColor white);
        
        /**
         * @brief Perform an incremental sweep step
         * @return true if sweep is complete, false if more steps needed
         */
        bool sweepStep();
        
        /**
         * @brief Check if sweep is complete
         * @return true if sweep is finished
         */
        bool isSweepComplete() const;
        
        /**
         * @brief Reset sweeper state for new cycle
         */
        void reset();
        
        /**
         * @brief Flip white colors for next collection cycle
         */
        void flipWhiteColors();
        
        /**
         * @brief Set finalizer callback function
         * @param callback Function to call for object finalization
         */
        void setFinalizerCallback(FinalizerCallback callback);
        
        /**
         * @brief Process all objects in finalization queue
         */
        void processFinalizers();
        
        /**
         * @brief Get current sweep statistics
         * @return Reference to current sweep statistics
         */
        const SweepStats& getStats() const { return stats; }
        
        /**
         * @brief Get current white color
         * @return Current white color
         */
        GCColor getCurrentWhite() const { return currentWhite; }
        
        /**
         * @brief Get next white color
         * @return Next white color
         */
        GCColor getNextWhite() const { return nextWhite; }
        
        /**
         * @brief Set maximum step size for incremental sweeping
         * @param stepSize Maximum objects to process per step
         */
        void setStepSize(usize stepSize) { maxStepSize = stepSize; }
        
        /**
         * @brief Get maximum step size
         * @return Maximum objects processed per step
         */
        usize getStepSize() const { return maxStepSize; }
        
    private:
        /**
         * @brief Check if an object should be freed
         * @param object Object to check
         * @return true if object should be freed
         */
        bool shouldFreeObject(GCObject* object) const;
        
        /**
         * @brief Free a single object
         * @param object Object to free
         * @param prev Previous object in list (for unlinking)
         * @return Next object in list
         */
        GCObject* freeObject(GCObject* object, GCObject* prev);
        
        /**
         * @brief Add object to finalization queue
         * @param object Object that needs finalization
         */
        void addToFinalizationQueue(GCObject* object);
        
        /**
         * @brief Remove object from string pool if it's a string
         * @param object Object to check and remove
         */
        void removeFromStringPool(GCObject* object);
        
        /**
         * @brief Update object color for next cycle
         * @param object Object to update
         */
        void updateObjectColor(GCObject* object);
        
        /**
         * @brief Calculate object memory footprint
         * @param object Object to calculate size for
         * @return Total memory used by object
         */
        usize calculateObjectSize(GCObject* object) const;
    };
    
    /**
     * @brief Helper function to determine if object is white
     * @param object Object to check
     * @param white Current white color
     * @return true if object is white
     */
    inline bool isWhite(const GCObject* object, GCColor white) {
        return object->getColor() == white;
    }
    
    /**
     * @brief Helper function to determine if object is black or gray
     * @param object Object to check
     * @return true if object is marked (black or gray)
     */
    inline bool isMarked(const GCObject* object) {
        GCColor color = object->getColor();
        return color == GCColor::Black || color == GCColor::Gray;
    }
    
    /**
     * @brief Helper function to flip white color
     * @param white Current white color
     * @return Opposite white color
     */
    inline GCColor flipWhite(GCColor white) {
        return (white == GCColor::White0) ? GCColor::White1 : GCColor::White0;
    }
}