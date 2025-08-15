#include "memory_pool.hpp"
#include "../core/gc_object.hpp"
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <new>
#include <algorithm>
#include <cassert>

namespace Lua {
    // Global memory pool instance
    GCMemoryPool* g_memoryPool = nullptr;

    // MemoryChunk implementation
    bool MemoryChunk::initialize() {
        // Allocate aligned memory
        constexpr usize alignment = alignof(std::max_align_t);
        memory = _aligned_malloc(size, alignment);
        if (!memory) {
            return false;
        }
        
        // Initialize free list
        char* current = static_cast<char*>(memory);
        freeList = current;
        freeCount = objectCount;
        
        // Link all objects in the free list
        for (usize i = 0; i < objectCount - 1; ++i) {
            void** next = reinterpret_cast<void**>(current);
            current += objectSize;
            *next = current;
        }
        
        // Last object points to null
        void** last = reinterpret_cast<void**>(current);
        *last = nullptr;
        
        return true;
    }
    
    void* MemoryChunk::allocate() {
        if (!freeList) {
            return nullptr;
        }
        
        void* result = freeList;
        freeList = *static_cast<void**>(freeList);
        --freeCount;
        
        return result;
    }
    
    void MemoryChunk::deallocate(void* ptr) {
        if (!ptr || !owns(ptr)) {
            return;
        }
        
        // Add to free list
        *static_cast<void**>(ptr) = freeList;
        freeList = ptr;
        ++freeCount;
    }
    
    bool MemoryChunk::owns(void* ptr) const {
        if (!ptr || !memory) {
            return false;
        }
        
        uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t startAddr = reinterpret_cast<uintptr_t>(memory);
        uintptr_t endAddr = startAddr + size;
        
        if (ptrAddr >= startAddr && ptrAddr < endAddr) {
            // Check if it's properly aligned to object boundary
            usize offset = ptrAddr - startAddr;
            return (offset % objectSize) == 0;
        }
        
        return false;
    }

    // FixedSizePool implementation
    FixedSizePool::FixedSizePool(FixedSizePool&& other) noexcept
        : objectSize(other.objectSize), chunkSize(other.chunkSize)
        , maxChunks(other.maxChunks), chunks(other.chunks)
        , currentChunk(other.currentChunk), totalChunks(other.totalChunks)
        , totalObjects(other.totalObjects), freeObjects(other.freeObjects)
        , allocCount(other.allocCount), deallocCount(other.deallocCount)
        , chunkAllocCount(other.chunkAllocCount) {
        other.chunks = nullptr;
        other.currentChunk = nullptr;
        other.totalChunks = 0;
        other.totalObjects = 0;
        other.freeObjects = 0;
    }
    
    FixedSizePool& FixedSizePool::operator=(FixedSizePool&& other) noexcept {
        if (this != &other) {
            cleanup();
            
            objectSize = other.objectSize;
            chunkSize = other.chunkSize;
            maxChunks = other.maxChunks;
            chunks = other.chunks;
            currentChunk = other.currentChunk;
            totalChunks = other.totalChunks;
            totalObjects = other.totalObjects;
            freeObjects = other.freeObjects;
            allocCount = other.allocCount;
            deallocCount = other.deallocCount;
            chunkAllocCount = other.chunkAllocCount;
            
            other.chunks = nullptr;
            other.currentChunk = nullptr;
            other.totalChunks = 0;
            other.totalObjects = 0;
            other.freeObjects = 0;
        }
        return *this;
    }
    
    void* FixedSizePool::allocate() {
        ScopedLock lock(poolMutex);
        
        // Try to allocate from current chunk
        if (currentChunk && !currentChunk->isFull()) {
            void* result = currentChunk->allocate();
            if (result) {
                --freeObjects;
                ++allocCount;
                return result;
            }
        }
        
        // Find a chunk with free space
        MemoryChunk* chunk = chunks;
        while (chunk) {
            if (!chunk->isFull()) {
                void* result = chunk->allocate();
                if (result) {
                    currentChunk = chunk;
                    --freeObjects;
                    ++allocCount;
                    return result;
                }
            }
            chunk = chunk->next;
        }
        
        // Need to allocate a new chunk
        if (totalChunks >= maxChunks) {
            return nullptr; // Reached maximum chunks
        }
        
        MemoryChunk* newChunk = allocateChunk();
        if (!newChunk) {
            return nullptr;
        }
        
        void* result = newChunk->allocate();
        if (result) {
            currentChunk = newChunk;
            --freeObjects;
            ++allocCount;
        }
        
        return result;
    }
    
    void FixedSizePool::deallocate(void* ptr) {
        if (!ptr) {
            return;
        }
        
        ScopedLock lock(poolMutex);
        
        MemoryChunk* chunk = findOwningChunk(ptr);
        if (chunk) {
            chunk->deallocate(ptr);
            ++freeObjects;
            ++deallocCount;
        }
    }
    
    bool FixedSizePool::owns(void* ptr) const {
        if (!ptr) {
            return false;
        }
        
        ScopedLock lock(poolMutex);
        return findOwningChunk(ptr) != nullptr;
    }
    
    void FixedSizePool::shrink() {
        ScopedLock lock(poolMutex);
        removeEmptyChunks();
    }
    
    void FixedSizePool::cleanup() {
        ScopedLock lock(poolMutex);
        
        MemoryChunk* chunk = chunks;
        while (chunk) {
            MemoryChunk* next = chunk->next;
            delete chunk;
            chunk = next;
        }
        
        chunks = nullptr;
        currentChunk = nullptr;
        totalChunks = 0;
        totalObjects = 0;
        freeObjects = 0;
    }
    
    bool FixedSizePool::canShrink() const {
        ScopedLock lock(poolMutex);
        
        MemoryChunk* chunk = chunks;
        while (chunk) {
            if (chunk->isEmpty()) {
                return true;
            }
            chunk = chunk->next;
        }
        
        return false;
    }
    
    MemoryChunk* FixedSizePool::allocateChunk() {
        auto chunk = std::make_unique<MemoryChunk>(chunkSize, objectSize);
        if (!chunk->initialize()) {
            return nullptr;
        }
        
        // Add to chunk list
        chunk->next = chunks;
        chunks = chunk.get();
        currentChunk = chunk.get();
        
        ++totalChunks;
        totalObjects += chunk->objectCount;
        freeObjects += chunk->freeCount;
        ++chunkAllocCount;
        
        return chunk.release();
    }
    
    MemoryChunk* FixedSizePool::findOwningChunk(void* ptr) const {
        MemoryChunk* chunk = chunks;
        while (chunk) {
            if (chunk->owns(ptr)) {
                return chunk;
            }
            chunk = chunk->next;
        }
        return nullptr;
    }
    
    void FixedSizePool::removeEmptyChunks() {
        MemoryChunk* prev = nullptr;
        MemoryChunk* chunk = chunks;
        
        while (chunk) {
            MemoryChunk* next = chunk->next;
            
            if (chunk->isEmpty()) {
                // Remove from list
                if (prev) {
                    prev->next = next;
                } else {
                    chunks = next;
                }
                
                // Update current chunk if needed
                if (currentChunk == chunk) {
                    currentChunk = chunks;
                }
                
                // Update counters
                --totalChunks;
                totalObjects -= chunk->objectCount;
                freeObjects -= chunk->freeCount;
                
                delete chunk;
            } else {
                prev = chunk;
            }
            
            chunk = next;
        }
    }

    // MemoryPoolManager implementation
    void MemoryPoolManager::initializeSizeClasses() {
        sizeClasses.clear();
        sizeToClassMap.clear();
        
        usize index = 0;
        
        // Small sizes: powers of 2 from MIN_SIZE_CLASS to MAX_SMALL_SIZE
        for (usize size = MIN_SIZE_CLASS; size <= MAX_SMALL_SIZE; size *= 2) {
            sizeClasses.push_back(size);
            sizeToClassMap[size] = index++;
        }
        
        // Medium sizes: increments of 1KB from MAX_SMALL_SIZE to MAX_MEDIUM_SIZE
        for (usize size = MAX_SMALL_SIZE + 1024; size <= MAX_MEDIUM_SIZE; size += 1024) {
            sizeClasses.push_back(size);
            sizeToClassMap[size] = index++;
        }
    }
    
    void MemoryPoolManager::initializePools() {
        pools.clear();
        pools.reserve(sizeClasses.size());
        
        for (usize sizeClass : sizeClasses) {
            // Adjust chunk size based on object size
            usize chunkSize = defaultConfig.chunkSize;
            if (sizeClass > 4096) {
                chunkSize = std::max(chunkSize, sizeClass * 16); // At least 16 objects per chunk
            }
            
            pools.emplace_back(std::make_unique<FixedSizePool>(sizeClass, chunkSize, defaultConfig.maxChunks));
        }
    }
    
    void* MemoryPoolManager::allocate(usize size) {
        if (size == 0) {
            return nullptr;
        }
        
        if (size >= LARGE_SIZE_THRESHOLD) {
            return allocateLarge(size);
        }
        
        usize poolIndex = getPoolIndex(size);
        if (poolIndex < pools.size()) {
            void* result = pools[poolIndex]->allocate();
            if (result) {
                usize allocatedSize = sizeClasses[poolIndex];
                updateStats(static_cast<isize>(allocatedSize));
                
                if (size <= MAX_SMALL_SIZE) {
                    smallAllocCount.fetch_add(1);
                } else {
                    mediumAllocCount.fetch_add(1);
                }
                
                return result;
            }
        }
        
        // Fallback to large allocation
        return allocateLarge(size);
    }
    
    void MemoryPoolManager::deallocate(void* ptr, usize size) {
        if (!ptr) {
            return;
        }
        
        // Try large objects first
        {
            ScopedLock lock(managerMutex);
            auto it = largeObjects.find(ptr);
            if (it != largeObjects.end()) {
                usize objectSize = it->second;
                largeObjects.erase(it);
                std::free(ptr);
                updateStats(-static_cast<isize>(objectSize), true);
                return;
            }
        }
        
        // Try pools
        if (size > 0 && size < LARGE_SIZE_THRESHOLD) {
            usize poolIndex = getPoolIndex(size);
            if (poolIndex < pools.size()) {
                if (pools[poolIndex]->owns(ptr)) {
                    pools[poolIndex]->deallocate(ptr);
                    usize allocatedSize = sizeClasses[poolIndex];
                    updateStats(-static_cast<isize>(allocatedSize));
                    return;
                }
            }
        }
        
        // Search all pools if size is unknown
        for (auto& pool : pools) {
            if (pool->owns(ptr)) {
                pool->deallocate(ptr);
                usize allocatedSize = pool->getObjectSize();
                updateStats(-static_cast<isize>(allocatedSize));
                return;
            }
        }
    }
    
    void* MemoryPoolManager::reallocate(void* ptr, usize oldSize, usize newSize) {
        if (!ptr) {
            return allocate(newSize);
        }
        
        if (newSize == 0) {
            deallocate(ptr, oldSize);
            return nullptr;
        }
        
        // Check if it's a large object
        {
            ScopedLock lock(managerMutex);
            auto it = largeObjects.find(ptr);
            if (it != largeObjects.end()) {
                usize currentSize = it->second;
                void* newPtr = std::realloc(ptr, newSize);
                if (newPtr) {
                    if (newPtr != ptr) {
                        largeObjects.erase(it);
                        largeObjects[newPtr] = newSize;
                    } else {
                        it->second = newSize;
                    }
                    updateStats(static_cast<isize>(newSize) - static_cast<isize>(currentSize), true);
                    return newPtr;
                }
                return nullptr;
            }
        }
        
        // For pool objects, allocate new and copy
        void* newPtr = allocate(newSize);
        if (newPtr && oldSize > 0) {
            std::memcpy(newPtr, ptr, std::min(oldSize, newSize));
            deallocate(ptr, oldSize);
        }
        
        return newPtr;
    }
    
    bool MemoryPoolManager::owns(void* ptr) const {
        if (!ptr) {
            return false;
        }
        
        // Check large objects
        {
            ScopedLock lock(managerMutex);
            if (largeObjects.find(ptr) != largeObjects.end()) {
                return true;
            }
        }
        
        // Check pools
        for (const auto& pool : pools) {
            if (pool->owns(ptr)) {
                return true;
            }
        }
        
        return false;
    }
    
    usize MemoryPoolManager::getSizeClass(usize size) const {
        usize poolIndex = getPoolIndex(size);
        if (poolIndex < sizeClasses.size()) {
            return sizeClasses[poolIndex];
        }
        return size; // Large object
    }
    
    usize MemoryPoolManager::getAllocatedSize(usize size) const {
        return getSizeClass(size);
    }
    
    void MemoryPoolManager::shrinkPools() {
        for (auto& pool : pools) {
            if (pool->canShrink()) {
                pool->shrink();
            }
        }
    }
    
    void MemoryPoolManager::cleanup() {
        // Clean up pools
        for (auto& pool : pools) {
            pool->cleanup();
        }
        pools.clear();
        
        // Clean up large objects
        ScopedLock lock(managerMutex);
        for (const auto& entry : largeObjects) {
            void* ptr = entry.first;
            std::free(ptr);
        }
        largeObjects.clear();
        
        // Reset statistics
        totalAllocated.store(0);
        totalFreed.store(0);
        currentUsage.store(0);
        smallAllocCount.store(0);
        mediumAllocCount.store(0);
        largeAllocCount.store(0);
    }
    
    void MemoryPoolManager::defragment() {
        // For now, just shrink pools
        // In a more sophisticated implementation, we could
        // compact memory and move objects
        shrinkPools();
    }
    
    void MemoryPoolManager::getPoolStats(Vec<usize>& sizes, Vec<usize>& usage, Vec<usize>& memory) const {
        sizes.clear();
        usage.clear();
        memory.clear();
        
        sizes.reserve(pools.size());
        usage.reserve(pools.size());
        memory.reserve(pools.size());
        
        for (const auto& pool : pools) {
            sizes.push_back(pool->getObjectSize());
            usage.push_back(pool->getUsedObjects());
            memory.push_back(pool->getMemoryUsage());
        }
    }
    
    void MemoryPoolManager::setPoolConfig(usize sizeClass, const PoolConfig& config) {
        usize poolIndex = getPoolIndex(sizeClass);
        if (poolIndex < pools.size()) {
            pools[poolIndex]->setMaxChunks(config.maxChunks);
        }
    }
    
    MemoryPoolManager::PoolConfig MemoryPoolManager::getPoolConfig(usize sizeClass) const {
        usize poolIndex = getPoolIndex(sizeClass);
        if (poolIndex < pools.size()) {
            return PoolConfig(defaultConfig.chunkSize, pools[poolIndex]->getMaxChunks());
        }
        return defaultConfig;
    }
    
    usize MemoryPoolManager::getPoolIndex(usize size) const {
        // Find the smallest size class that can accommodate the size
        auto it = std::lower_bound(sizeClasses.begin(), sizeClasses.end(), size);
        if (it != sizeClasses.end()) {
            return static_cast<usize>(it - sizeClasses.begin());
        }
        return sizeClasses.size(); // Indicates large object
    }
    
    void* MemoryPoolManager::allocateLarge(usize size) {
        void* ptr = std::malloc(size);
        if (ptr) {
            ScopedLock lock(managerMutex);
            largeObjects[ptr] = size;
            updateStats(static_cast<isize>(size), true);
            largeAllocCount.fetch_add(1);
        }
        return ptr;
    }
    
    void MemoryPoolManager::deallocateLarge(void* ptr) {
        ScopedLock lock(managerMutex);
        auto it = largeObjects.find(ptr);
        if (it != largeObjects.end()) {
            usize size = it->second;
            largeObjects.erase(it);
            std::free(ptr);
            updateStats(-static_cast<isize>(size), true);
        }
    }
    
    void MemoryPoolManager::updateStats(isize delta, bool isLarge) {
        if (delta > 0) {
            totalAllocated.fetch_add(static_cast<usize>(delta));
            currentUsage.fetch_add(static_cast<usize>(delta));
        } else {
            totalFreed.fetch_add(static_cast<usize>(-delta));
            currentUsage.fetch_sub(static_cast<usize>(-delta));
        }
    }
    
    usize MemoryPoolManager::roundUpToSizeClass(usize size) const {
        auto it = std::lower_bound(sizeClasses.begin(), sizeClasses.end(), size);
        if (it != sizeClasses.end()) {
            return *it;
        }
        return size;
    }

    // GCMemoryPool implementation
    void* GCMemoryPool::allocateRaw(usize size, GCObjectType type) {
        void* ptr = poolManager.allocate(size);
        if (ptr) {
            // Track GC object type
            ScopedLock lock(gcMutex);
            gcObjects[ptr] = type;
            
            updateGCStats(static_cast<isize>(size));
        }
        return ptr;
    }
    
    void GCMemoryPool::deallocate(void* ptr) {
        if (!ptr) {
            return;
        }
        
        // Get allocated size for statistics
        usize size = 0;
        {
            ScopedLock lock(gcMutex);
            auto it = gcObjects.find(ptr);
            if (it != gcObjects.end()) {
                gcObjects.erase(it);
                // Estimate size from pool manager
                size = poolManager.getAllocatedSize(1); // Minimum estimate
            }
        }
        
        poolManager.deallocate(ptr);
        
        if (size > 0) {
            updateGCStats(-static_cast<isize>(size));
        }
    }
    
    void* GCMemoryPool::reallocate(void* ptr, usize newSize) {
        if (!ptr) {
            return allocateRaw(newSize);
        }
        
        if (newSize == 0) {
            deallocate(ptr);
            return nullptr;
        }
        
        // Get old type
        GCObjectType type = GCObjectType::String;
        {
            ScopedLock lock(gcMutex);
            auto it = gcObjects.find(ptr);
            if (it != gcObjects.end()) {
                type = it->second;
                gcObjects.erase(it);
            }
        }
        
        void* newPtr = poolManager.reallocate(ptr, 0, newSize); // oldSize unknown
        if (newPtr) {
            ScopedLock lock(gcMutex);
            gcObjects[newPtr] = type;
        }
        
        return newPtr;
    }
    
    void GCMemoryPool::handleMemoryPressure() {
        // Trigger GC if available
        if (gc && shouldTriggerGC()) {
            // GC will be triggered by the caller
        }
        
        // Shrink pools to free unused memory
        poolManager.shrinkPools();
    }
    
    void GCMemoryPool::registerWithGC(GCObject* obj, GCObjectType type) {
        if (gc && obj) {
            // Set object type
            //obj->setObjectType(type); TODO
            
            // Register with GC (implementation depends on GC interface)
            // This would typically add the object to the GC's object list
        }
    }
    
    void GCMemoryPool::updateGCStats(isize delta) {
        if (stats) {
            if (delta > 0) {
                stats->totalAllocated += static_cast<usize>(delta);
                stats->currentUsage += static_cast<usize>(delta);
            } else {
                stats->totalFreed += static_cast<usize>(-delta);
                stats->currentUsage -= static_cast<usize>(-delta);
            }
            
            stats->updatePeakUsage();
        }
    }
}