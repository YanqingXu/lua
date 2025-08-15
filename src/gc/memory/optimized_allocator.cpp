#include "optimized_allocator.hpp"
#include "../core/gc_object.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <new>
#include <chrono>

namespace Lua {
    // Global optimized allocator instance
    OptimizedGCAllocator* g_optimizedGCAllocator = nullptr;

    // HybridObjectPool implementation
    void* HybridObjectPool::allocate(GCObjectType type, bool isGCObject) {
    ScopedLock lock{poolMutex};
        
        void* result = nullptr;
        
        if (isGCObject) {
            // Allocate from GC pool (includes header space)
            result = gcPool->allocate();
            if (result) {
                // Initialize header
                OptimizedMemoryHeader* header = new(result) OptimizedMemoryHeader(type);
                
                // Return pointer after header
                result = reinterpret_cast<char*>(result) + sizeof(OptimizedMemoryHeader);
                gcAllocations.fetch_add(1);
            }
        } else {
            // Allocate from fast pool (no header)
            result = fastPool->allocate();
            if (result) {
                fastAllocations.fetch_add(1);
            }
        }
        
        return result;
    }
    
    void HybridObjectPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    ScopedLock lock{poolMutex};
        
        // Check which pool owns this pointer
        if (isGCObject(ptr)) {
            // Get original pointer (before header)
            void* originalPtr = reinterpret_cast<char*>(ptr) - sizeof(OptimizedMemoryHeader);
            gcPool->deallocate(originalPtr);
            gcDeallocations.fetch_add(1);
        } else if (fastPool->owns(ptr)) {
            fastPool->deallocate(ptr);
            fastDeallocations.fetch_add(1);
        }
    }
    
    bool HybridObjectPool::owns(void* ptr) const {
    if (!ptr) return false;
    
    ScopedLock lock{poolMutex};
        
        // Check fast pool first (more common)
        if (fastPool->owns(ptr)) {
            return true;
        }
        
        // Check GC pool (need to account for header offset)
        void* headerPtr = reinterpret_cast<char*>(ptr) - sizeof(OptimizedMemoryHeader);
        return gcPool->owns(headerPtr);
    }
    
    GCObjectType HybridObjectPool::getObjectType(void* ptr) const {
        OptimizedMemoryHeader* header = getHeader(ptr);
        return header ? header->objectType : GCObjectType::String;
    }
    
    u16 HybridObjectPool::getGCFlags(void* ptr) const {
        OptimizedMemoryHeader* header = getHeader(ptr);
        return header ? header->flags : 0;
    }
    
    void HybridObjectPool::setGCFlags(void* ptr, u16 flags) {
        OptimizedMemoryHeader* header = getHeader(ptr);
        if (header) {
            header->flags = flags;
        }
    }
    
    OptimizedMemoryHeader* HybridObjectPool::getHeader(void* ptr) const {
        if (!ptr || !isGCObject(ptr)) {
            return nullptr;
        }
        
        return reinterpret_cast<OptimizedMemoryHeader*>(
            reinterpret_cast<char*>(ptr) - sizeof(OptimizedMemoryHeader)
        );
    }
    
    bool HybridObjectPool::isGCObject(void* ptr) const {
        if (!ptr) return false;
        
        // Check if the header pointer would be valid in GC pool
        void* headerPtr = reinterpret_cast<char*>(ptr) - sizeof(OptimizedMemoryHeader);
        return gcPool->owns(headerPtr);
    }

    // OptimizedGCAllocator implementation
    void OptimizedGCAllocator::initializeObjectPools() {
        for (usize i = 0; i < NUM_POOLS; ++i) {
            usize poolSize = MIN_POOL_SIZE << i;
            
            // Use larger chunks for bigger objects to reduce overhead
            usize chunkSize = std::max(64 * 1024ULL, poolSize * 256);
            
            //objectPools[i] = UPtr<HybridObjectPool>(poolSize, chunkSize); TODO
        }
    }
    
    void OptimizedGCAllocator::initializeLargeObjectManager() {
        // TODO: Implement large object manager
        // MemoryPoolConfig poolConfig;
        // poolConfig.initialPoolCount = 8;
        // poolConfig.maxPoolCount = 64;
        // poolConfig.chunkSize = 1024 * 1024; // 1MB chunks for large objects
        // poolConfig.enableStatistics = true;
        // poolConfig.enableShrinking = true;
        
        // largeObjectManager = UPtr<MemoryPoolManager>(poolConfig);
    }
    
    usize OptimizedGCAllocator::getPoolIndex(usize size) const {
        if (size <= MIN_POOL_SIZE) {
            return 0;
        }
        
        // Find the smallest pool that can fit the size
        usize adjustedSize = size - 1; // For exact power of 2 sizes
        usize index = 0;
        usize poolSize = MIN_POOL_SIZE;
        
        while (poolSize < adjustedSize && index < NUM_POOLS - 1) {
            poolSize <<= 1;
            index++;
        }
        
        return index;
    }
    
    void* OptimizedGCAllocator::allocateRaw(usize size, GCObjectType type, bool isGCObject) {
        if (size == 0) {
            return nullptr;
        }
        
        // Check memory limits
        if (!checkMemoryLimits(size)) {
            handleMemoryPressure();
            if (!checkMemoryLimits(size)) {
                return nullptr;
            }
        }
        
        void* ptr = nullptr;
        
        if (size <= MAX_POOL_SIZE) {
            ptr = allocateFromPool(size, type, isGCObject);
            if (ptr) {
                poolHits.fetch_add(1);
            }
        }
        
        if (!ptr) {
            ptr = allocateLargeObject(size, type, isGCObject);
            if (ptr) {
                poolMisses.fetch_add(1);
            }
        }
        
        if (ptr) {
            updateStats(static_cast<isize>(size), isGCObject);
            updateAllocationPattern(size, isGCObject);
            
            // Check if GC should be triggered
            if (shouldTriggerGC() && gc) {
                // Note: Actual GC triggering would be handled by the caller
                // to avoid recursive allocation during GC
            }
        }
        
        return ptr;
    }
    
    void* OptimizedGCAllocator::allocateFromPool(usize size, GCObjectType type, bool isGCObject) {
        usize poolIndex = getPoolIndex(size);
        if (poolIndex >= NUM_POOLS) {
            return nullptr;
        }
        
        return objectPools[poolIndex]->allocate(type, isGCObject);
    }
    
    void* OptimizedGCAllocator::allocateLargeObject(usize size, GCObjectType type, bool isGCObject) {
        ScopedLock lock{allocatorMutex};
        
        if (isGCObject) {
            // For GC objects, we need to store metadata
            usize totalSize = size + sizeof(OptimizedMemoryHeader);
            void* rawPtr = largeObjectManager->allocate(totalSize);
            if (!rawPtr) {
                return nullptr;
            }
            
            // Initialize header
            OptimizedMemoryHeader* header = new(rawPtr) OptimizedMemoryHeader(type);
            
            // Return pointer after header
            return reinterpret_cast<char*>(rawPtr) + sizeof(OptimizedMemoryHeader);
        } else {
            // For non-GC objects, allocate directly
            return largeObjectManager->allocate(size);
        }
    }
    
    void OptimizedGCAllocator::deallocate(void* ptr) {
        if (!ptr) return;
        
        bool foundInPool = false;
        bool wasGCObject = false;
        
        // Try pools first
        for (auto& pool : objectPools) {
            if (pool && pool->owns(ptr)) {
                // Determine if it was a GC object by checking the pool
                //wasGCObject = pool->isGCObject(ptr); TODO
                pool->deallocate(ptr);
                foundInPool = true;
                break;
            }
        }
        
        if (!foundInPool) {
            // Try large object manager
            ScopedLock lock{allocatorMutex};
            
            // Check if it's a GC object by looking for header
            void* headerPtr = reinterpret_cast<char*>(ptr) - sizeof(OptimizedMemoryHeader);
            if (largeObjectManager->owns(headerPtr)) {
                // It's a GC object, deallocate from header position
                largeObjectManager->deallocate(headerPtr);
                wasGCObject = true;
            } else if (largeObjectManager->owns(ptr)) {
                // It's a non-GC object, deallocate directly
                largeObjectManager->deallocate(ptr);
                wasGCObject = false;
            }
        }
        
        // Update statistics (we'd need to store size information for accurate stats)
        // This is a limitation that could be addressed by storing size in headers
        if (wasGCObject) {
            gcObjectCount.fetch_sub(1);
        } else {
            fastObjectCount.fetch_sub(1);
        }
    }
    
    void* OptimizedGCAllocator::reallocate(void* ptr, usize newSize) {
        if (!ptr) {
            return allocateRaw(newSize);
        }
        
        if (newSize == 0) {
            deallocate(ptr);
            return nullptr;
        }
        
        // For simplicity, allocate new memory and copy
        // A more sophisticated implementation would try to expand in place
        void* newPtr = allocateRaw(newSize);
        if (!newPtr) {
            return nullptr;
        }
        
        // We'd need to know the old size to copy properly
        // This could be improved by storing size information
        
        deallocate(ptr);
        return newPtr;
    }
    
    void OptimizedGCAllocator::registerWithGC(GCObject* obj) {
        // This would be implemented when we have the GarbageCollector class
        // For now, it's a placeholder
        (void)obj; // Suppress unused parameter warning
    }
    
    void OptimizedGCAllocator::updateStats(isize delta, bool isGCObject) {
        if (delta > 0) {
            totalAllocated.fetch_add(static_cast<usize>(delta));
            currentUsage.fetch_add(static_cast<usize>(delta));
            
            if (isGCObject) {
                gcObjectCount.fetch_add(1);
            } else {
                fastObjectCount.fetch_add(1);
            }
        } else {
            totalFreed.fetch_add(static_cast<usize>(-delta));
            currentUsage.fetch_sub(static_cast<usize>(-delta));
        }
        
        // Update GC stats if available
        if (stats) {
            stats->totalAllocated = totalAllocated.load();
            stats->totalFreed = totalFreed.load();
            stats->currentUsage = currentUsage.load();
            stats->updatePeakUsage();
        }
    }
    
    bool OptimizedGCAllocator::checkMemoryLimits(usize requestedSize) const {
        usize currentMem = currentUsage.load();
        usize maxThreshold = config.maxThreshold;
        
        return (currentMem + requestedSize) <= maxThreshold;
    }
    
    void OptimizedGCAllocator::updateAllocationPattern(usize size, bool isGCObject) {
        // Simple pattern tracking for adaptive tuning
        usize pattern = (size << 1) | (isGCObject ? 1 : 0);
        allocationPattern.store(pattern);
        
        // Trigger tuning periodically
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        u64 currentTime = static_cast<u64>(now);
        u64 lastTuning = lastTuningTime.load();
        
        // Tune every 10 seconds
        if (currentTime - lastTuning > 10000000000ULL) {
            if (lastTuningTime.compare_exchange_weak(lastTuning, currentTime)) {
                tunePoolSizes();
            }
        }
    }
    
    void OptimizedGCAllocator::getPoolStats(Vec<usize>& poolUsage, Vec<usize>& poolMemory) const {
        poolUsage.clear();
        poolMemory.clear();
        poolUsage.reserve(NUM_POOLS);
        poolMemory.reserve(NUM_POOLS);
        
        for (const auto& pool : objectPools) {
            if (pool) {
                poolUsage.push_back(pool->getUsedObjects());
                poolMemory.push_back(pool->getMemoryUsage());
            } else {
                poolUsage.push_back(0);
                poolMemory.push_back(0);
            }
        }
    }
    
    void OptimizedGCAllocator::getDetailedStats(HashMap<Str, usize>& detailedStats) const {
        detailedStats.clear();
        
        detailedStats["total_allocated"] = totalAllocated.load();
        detailedStats["total_freed"] = totalFreed.load();
        detailedStats["current_usage"] = currentUsage.load();
        detailedStats["gc_threshold"] = gcThreshold.load();
        detailedStats["pool_hits"] = poolHits.load();
        detailedStats["pool_misses"] = poolMisses.load();
        detailedStats["gc_object_count"] = gcObjectCount.load();
        detailedStats["fast_object_count"] = fastObjectCount.load();
        
        // Pool-specific stats
        for (usize i = 0; i < NUM_POOLS; ++i) {
            if (objectPools[i]) {
                Str prefix = "pool_" + std::to_string(i) + "_";
                detailedStats[prefix + "used_objects"] = objectPools[i]->getUsedObjects();
                detailedStats[prefix + "free_objects"] = objectPools[i]->getFreeObjects();
                detailedStats[prefix + "memory_usage"] = objectPools[i]->getMemoryUsage();
                detailedStats[prefix + "gc_allocations"] = objectPools[i]->getGCAllocations();
                detailedStats[prefix + "fast_allocations"] = objectPools[i]->getFastAllocations();
            }
        }
        
        // Large object manager stats
        if (largeObjectManager) {
            // TODO
            //auto poolStats = largeObjectManager->getPoolStats();
            //detailedStats["large_object_pools"] = poolStats.size();
            
            // usize totalLargeMemory = 0;
            // usize totalLargeObjects = 0;
            // for (const auto& stat : poolStats) {
            //     totalLargeMemory += stat.memoryUsage;
            //     totalLargeObjects += stat.usedObjects;
            // }
            // detailedStats["large_object_memory"] = totalLargeMemory;
            // detailedStats["large_object_count"] = totalLargeObjects;
        }
    }
    
    void OptimizedGCAllocator::handleMemoryPressure() {
        // Force garbage collection if possible
        if (gc) {
            // This would trigger GC - implementation depends on GC interface
        }
        
        // Shrink pools to free unused memory
        for (auto& pool : objectPools) {
            if (pool) {
                pool->shrink();
            }
        }
        
        if (largeObjectManager) {
            //largeObjectManager->handleMemoryPressure(); TODO
        }
        
        // Reduce GC threshold temporarily
        usize currentThreshold = gcThreshold.load();
        usize newThreshold = std::max(currentThreshold / 2, config.initialThreshold / 2);
        gcThreshold.store(newThreshold);
    }
    
    void OptimizedGCAllocator::defragment() {
        ScopedLock lock{allocatorMutex};
        
        // Defragment pools
        for (auto& pool : objectPools) {
            if (pool) {
                pool->shrink(); // Remove empty chunks
            }
        }
        
        // Defragment large object manager
        if (largeObjectManager) {
            largeObjectManager->defragment();
        }
    }
    
    void OptimizedGCAllocator::tunePoolSizes() {
        // Adaptive tuning based on allocation patterns
        // This is a simplified implementation
        
        usize pattern = allocationPattern.load();
        usize size = pattern >> 1;
        bool isGCObject = (pattern & 1) != 0;
        
        // Adjust pool configurations based on recent patterns
        if (size > 0 && size <= MAX_POOL_SIZE) {
            usize poolIndex = getPoolIndex(size);
            if (poolIndex < NUM_POOLS && objectPools[poolIndex]) {
                // Could adjust chunk sizes, max chunks, etc. based on usage
                // For now, this is a placeholder for future optimization
            }
        }
    }
    
    GCObjectType OptimizedGCAllocator::getObjectType(void* ptr) const {
        if (!ptr) return GCObjectType::String;
        
        // Try pools first
        for (const auto& pool : objectPools) {
            if (pool && pool->owns(ptr)) {
                return pool->getObjectType(ptr);
            }
        }
        
        // Try large objects
        ScopedLock lock{allocatorMutex};
        void* headerPtr = reinterpret_cast<char*>(ptr) - sizeof(OptimizedMemoryHeader);
        if (largeObjectManager && largeObjectManager->owns(headerPtr)) {
            OptimizedMemoryHeader* header = reinterpret_cast<OptimizedMemoryHeader*>(headerPtr);
            return header->objectType;
        }
        
        return GCObjectType::String; // Default
    }
    
    u16 OptimizedGCAllocator::getGCFlags(void* ptr) const {
        if (!ptr) return 0;
        
        // Try pools first
        for (const auto& pool : objectPools) {
            if (pool && pool->owns(ptr)) {
                return pool->getGCFlags(ptr);
            }
        }
        
        // Try large objects
        ScopedLock lock{allocatorMutex};
        void* headerPtr = reinterpret_cast<char*>(ptr) - sizeof(OptimizedMemoryHeader);
        if (largeObjectManager && largeObjectManager->owns(headerPtr)) {
            OptimizedMemoryHeader* header = reinterpret_cast<OptimizedMemoryHeader*>(headerPtr);
            return header->flags;
        }
        
        return 0;
    }
    
    void OptimizedGCAllocator::setGCFlags(void* ptr, u16 flags) {
        if (!ptr) return;
        
        // Try pools first
        for (const auto& pool : objectPools) {
            if (pool && pool->owns(ptr)) {
                pool->setGCFlags(ptr, flags);
                return;
            }
        }
        
        // Try large objects
        ScopedLock lock{allocatorMutex};
        void* headerPtr = reinterpret_cast<char*>(ptr) - sizeof(OptimizedMemoryHeader);
        if (largeObjectManager && largeObjectManager->owns(headerPtr)) {
            OptimizedMemoryHeader* header = reinterpret_cast<OptimizedMemoryHeader*>(headerPtr);
            header->flags = flags;
        }
    }
}