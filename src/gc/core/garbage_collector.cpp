#include "garbage_collector.hpp"
#include "../../vm/lua_state.hpp"
#include <chrono>
#include <iostream>
#include <limits>

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
        gcState = GCState::Pause;  // 暂时简化，不使用Finalize状态
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

    // === Lua 5.1 Compatible Incremental GC Implementation ===

    void GarbageCollector::step(LuaState* L) {
        // 暂时简化实现，避免编译错误
        if (!L) return;

        // 简单的GC触发 - 当分配内存超过阈值时触发
        if (totalAllocated > gcThreshold) {
            collectGarbage();
        }
    }

    void GarbageCollector::fullGC(LuaState* L) {
        // 暂时简化实现，避免编译错误
        if (!L) return;

        // 执行完整的垃圾回收
        collectGarbage();

        // 更新阈值
        updateThreshold();
    }

    isize GarbageCollector::singleStep() {
        // 暂时简化实现，避免编译错误
        if (gcState == GCState::Pause) {
            return markRoot();
        } else if (gcState == GCState::Propagate) {
            if (marker.hasGrayObjects()) {
                return propagateMarkStep();
            } else {
                return atomicStep();
            }
        } else if (gcState == GCState::SweepString) {
            return sweepStringStep();
        } else if (gcState == GCState::Sweep) {
            return sweepObjectStep();
        } else if (gcState == static_cast<GCState>(4)) {  // GCState::Finalize
            return finalizeStep();
        }
        return 0;
    }

    void GarbageCollector::addToGrayAgain(GCObject* obj) {
        if (!obj) return;

        // 将对象添加到grayagain列表头部
        // 这里需要使用对象的gclist字段，类似官方实现
        // 暂时简化实现，实际需要根据对象类型处理
        obj->setNext(grayAgainList);
        grayAgainList = obj;
    }

    void GarbageCollector::updateThreshold() {
        // 对应官方setthreshold(g) = (g->estimate/100) * g->gcpause
        gcThreshold = (estimate / 100) * config.gcpause;
        if (gcThreshold < 1024 * 1024) { // 最小1MB
            gcThreshold = 1024 * 1024;
        }
    }

    // === 私有辅助方法实现 ===

    isize GarbageCollector::markRoot() {
        // 暂时简化实现，避免编译错误
        marker.reset();
        grayAgainList = nullptr;
        weakList = nullptr;

        gcState = GCState::Propagate;
        return 0;
    }

    isize GarbageCollector::propagateMarkStep() {
        // 暂时简化实现，避免编译错误
        gcState = GCState::SweepString;
        return 1;
    }

    isize GarbageCollector::atomicStep() {
        // 暂时简化实现，避免编译错误
        currentWhite = (currentWhite == GCColor::White0) ? GCColor::White1 : GCColor::White0;
        sweepStringPos = 0;
        gcState = GCState::SweepString;
        estimate = totalAllocated;
        return 1;
    }

    isize GarbageCollector::sweepStringStep() {
        // 暂时简化实现，避免编译错误
        gcState = GCState::Sweep;
        return 10;
    }

    isize GarbageCollector::sweepObjectStep() {
        // 暂时简化实现，避免编译错误
        gcState = static_cast<GCState>(4);  // GCState::Finalize
        return 10;
    }

    isize GarbageCollector::finalizeStep() {
        // 暂时简化实现，避免编译错误
        gcState = GCState::Pause;
        return 100;
    }

    // === 全局GC函数实现 - 确保链接成功 ===

    /**
     * @brief 简化的luaC_step实现 - 确保编译成功
     */
    void luaC_step(LuaState* L) {
        // 简化的stub实现 - 暂时不执行任何实际的GC操作
        if (!L) return;
        // 暂时不执行任何GC步骤
    }

    // === WriteBarrier函数实现 - 确保链接成功 ===

    namespace WriteBarrier {
        /**
         * @brief 简化的barrierForward实现 - 确保编译成功
         */
        void barrierForward(LuaState* L, GCObject* parent, GCObject* child) {
            // 简化的stub实现 - 确保编译成功
            if (!L || !parent || !child) {
                return;
            }
            // 暂时不执行任何实际的屏障操作
        }

        /**
         * @brief 简化的barrierBackward实现 - 确保编译成功
         */
        void barrierBackward(LuaState* L, GCObject* obj) {
            // 简化的stub实现 - 确保编译成功
            if (!L || !obj) {
                return;
            }
            // 暂时不执行任何实际的屏障操作
        }

        /**
         * @brief 简化的needsBarrier实现 - 确保编译成功
         */
        bool needsBarrier(GCObject* parent, GCObject* child) {
            // 简化的stub实现 - 暂时总是返回false
            return false;
        }
    }
}