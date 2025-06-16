#include "garbage_collector.hpp"
#include "../../vm/state.hpp"
#include <chrono>
#include <iostream>

namespace Lua {
    void GarbageCollector::markObject(GCObject* obj) {
        if (!obj) {
            return;
        }
        
        // Check if object is already marked
        if (obj->getColor() == GCColor::Black || obj->getColor() == GCColor::Gray) {
            return;
        }
        
        // Mark object as gray (reachable but not processed)
        obj->setColor(GCColor::Gray);
        
        // Mark all references from this object
        obj->markReferences(this);
        
        // Mark object as black (fully processed)
        obj->setColor(GCColor::Black);
    }
    
    void GarbageCollector::collectGarbage() {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Update GC state
        gcState = GCState::Propagate;
        
        // Phase 1: Mark all reachable objects
        markPhase();
        
        // Phase 2: Sweep unreachable objects
        gcState = GCState::Sweep;
        sweepPhase();
        
        // Phase 3: Finalization and cleanup
        gcState = GCState::Finalize;
        flipWhiteColors();
        
        // Update statistics
        auto endTime = std::chrono::high_resolution_clock::now();
        auto gcTime = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count();
        
        stats.gcCycles++;
        stats.totalGCTime += gcTime;
        stats.avgPauseTime = stats.totalGCTime / stats.gcCycles;
        
        if (gcTime > static_cast<i64>(stats.maxPauseTime)) {
            stats.maxPauseTime = gcTime;
        }
        
        // Update peak memory usage
        stats.updatePeakUsage();
        
        // Adjust GC threshold based on current memory usage
        gcThreshold = std::max(stats.currentUsage * 2, static_cast<usize>(1024 * 1024));
        
        // Return to pause state
        gcState = GCState::Pause;
        
        // Optional: Log GC statistics in debug mode
        #ifdef DEBUG_GC
        std::cout << "[GC] Collection completed in " << gcTime << "μs\n";
        std::cout << "     Objects collected: " << sweeper.getStats().objectsFreed << "\n";
        std::cout << "     Memory freed: " << sweeper.getStats().bytesFreed << " bytes\n";
        std::cout << "     Live objects: " << stats.liveObjects << "\n";
        std::cout << "     Current usage: " << stats.currentUsage << " bytes\n";
        #endif
    }
    
    bool GarbageCollector::shouldCollect() const {
        // Don't trigger GC if already running
        if (gcState != GCState::Pause) {
            return false;
        }
        
        // Trigger GC if we've exceeded the threshold
        if (totalAllocated >= gcThreshold) {
            return true;
        }
        
        // Trigger GC if we have too many objects relative to memory usage
        if (stats.liveObjects > 0 && totalAllocated / stats.liveObjects > 1024) {
            return true;
        }
        
        return false;
    }
    
    void GarbageCollector::registerObject(GCObject* obj) {
        if (!obj) return;
        
        // Add object to the linked list of all objects
        obj->setNext(allObjectsHead);
        if (allObjectsHead) {
            allObjectsHead->setPrev(obj);
        }
        allObjectsHead = obj;
        obj->setPrev(nullptr);
        
        // Update statistics
        stats.totalObjects++;
        stats.liveObjects++;
        
        usize objSize = obj->getSize() + obj->getAdditionalSize();
        stats.totalAllocated += objSize;
        stats.currentUsage += objSize;
        totalAllocated += objSize;
    }
    
    void GarbageCollector::updateAllocatedMemory(isize delta) {
        if (delta > 0) {
            stats.totalAllocated += static_cast<usize>(delta);
            stats.currentUsage += static_cast<usize>(delta);
            totalAllocated += static_cast<usize>(delta);
        } else if (delta < 0) {
            usize absDelta = static_cast<usize>(-delta);
            stats.totalFreed += absDelta;
            if (stats.currentUsage >= absDelta) {
                stats.currentUsage -= absDelta;
            } else {
                stats.currentUsage = 0;
            }
            if (totalAllocated >= absDelta) {
                totalAllocated -= absDelta;
            } else {
                totalAllocated = 0;
            }
        }
        
        stats.updatePeakUsage();
    }
    
    std::vector<GCObject*> GarbageCollector::collectRootObjects() {
        std::vector<GCObject*> roots;
        
        // Add the Lua state as a root object
        if (luaState) {
            roots.push_back(luaState);
        }
        
        // Add any globally fixed objects
        GCObject* current = allObjectsHead;
        while (current) {
            if (current->isFixed()) {
                roots.push_back(current);
            }
            current = current->getNext();
        }
        
        return roots;
    }
    
    void GarbageCollector::markPhase() {
        // Collect root objects
        std::vector<GCObject*> rootObjects = collectRootObjects();
        
        // Reset marker state
        marker.reset();
        
        // Mark from all root objects
        marker.markFromRoots(rootObjects, currentWhite);
        
        // Mark all strings in the string pool
        StringPool& stringPool = StringPool::getInstance();
        stringPool.markAll(this);
        
        // Update statistics
        stats.liveObjects = marker.getMarkedObjectCount();
    }
    
    void GarbageCollector::sweepPhase() {
        // Perform sweep operation
        allObjectsHead = sweeper.sweepAll(allObjectsHead, currentWhite);
        
        // Update statistics from sweep results
        const auto& sweepStats = sweeper.getStats();
        stats.collectedObjects += sweepStats.objectsFreed;
        stats.totalFreed += sweepStats.bytesFreed;
        stats.currentUsage -= sweepStats.bytesFreed;
        stats.liveObjects = sweepStats.objectsKept;
        
        // Update total allocated to reflect actual current usage
        totalAllocated = stats.currentUsage;
    }
    
    void GarbageCollector::flipWhiteColors() {
        // Flip white colors for next collection cycle
        currentWhite = (currentWhite == GCColor::White0) ? GCColor::White1 : GCColor::White0;
        
        // Update all remaining objects to use the new white color
        GCObject* current = allObjectsHead;
        while (current) {
            if (current->getColor() == GCColor::Black) {
                // Black objects become the new white for next cycle
                current->setColor(currentWhite);
            }
            current = current->getNext();
        }
    }
}