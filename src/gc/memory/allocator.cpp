#include "allocator.hpp"
#include "../core/gc_object.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <new>

namespace Lua {
    // Global allocator instance
    GCAllocator* g_gcAllocator = nullptr;

    // ObjectPool implementation
    void* ObjectPool::allocate(GCObjectType type) {
        std::lock_guard<std::mutex> lock(poolMutex);
    
        if (!freeList) {
            allocateChunk();
        }
    
        if (!freeList) {
            return nullptr; // Out of memory
        }
    
        MemoryBlockHeader* block = freeList;
        freeList = freeList->next;
        if (freeList) {
            freeList->prev = nullptr;
        }
    
        // Initialize block header
        block->objectType = type;
        block->isGCObject = true;
        block->next = nullptr;
        block->prev = nullptr;
    
        freeObjects--;
        return reinterpret_cast<void*>(reinterpret_cast<char*>(block) + sizeof(MemoryBlockHeader));
    }

    void ObjectPool::deallocate(void* ptr) {
        if (!ptr) return;
    
        std::lock_guard<std::mutex> lock(poolMutex);
    
        // Get block header
        MemoryBlockHeader* block = reinterpret_cast<MemoryBlockHeader*>(
            reinterpret_cast<char*>(ptr) - sizeof(MemoryBlockHeader)
        );
    
        // Add to free list
        block->next = freeList;
        block->prev = nullptr;
        if (freeList) {
            freeList->prev = block;
        }
        freeList = block;
    
        freeObjects++;
    }

    bool ObjectPool::owns(void* ptr) const {
        if (!ptr) return false;
    
        std::lock_guard<std::mutex> lock(poolMutex);
    
        for (void* chunk : chunks) {
            char* chunkStart = reinterpret_cast<char*>(chunk);
            char* chunkEnd = chunkStart + chunkSize;
            char* ptrAddr = reinterpret_cast<char*>(ptr);
        
            if (ptrAddr >= chunkStart && ptrAddr < chunkEnd) {
                // Check if it's properly aligned
                usize offset = ptrAddr - chunkStart;
                return (offset % objectSize) == sizeof(MemoryBlockHeader);
            }
        }
    
        return false;
    }

    void ObjectPool::allocateChunk() {
        void* chunk = _aligned_malloc(chunkSize, alignof(std::max_align_t));
        if (!chunk) {
            return; // Out of memory
        }
    
        chunks.push_back(chunk);
        initializeFreeList(chunk);
        totalObjects += objectsPerChunk;
        freeObjects += objectsPerChunk;
    }

    void ObjectPool::initializeFreeList(void* chunk) {
        char* current = reinterpret_cast<char*>(chunk);
        char* end = current + chunkSize;
    
        MemoryBlockHeader* prevBlock = nullptr;
    
        while (current + objectSize <= end) {
            MemoryBlockHeader* block = reinterpret_cast<MemoryBlockHeader*>(current);
            new(block) MemoryBlockHeader(objectSize - sizeof(MemoryBlockHeader), 
                                       GCObjectType::String, alignof(std::max_align_t), true);
        
            block->prev = prevBlock;
            block->next = nullptr;
        
            if (prevBlock) {
                prevBlock->next = block;
            } else {
                freeList = block;
            }
        
            prevBlock = block;
            current += objectSize;
        }
    }

    // GCAllocator implementation
    void GCAllocator::initializeObjectPools() {
        // Calculate properly aligned header size
        constexpr usize headerAlign = alignof(MemoryBlockHeader);
        constexpr usize headerSize = (sizeof(MemoryBlockHeader) + headerAlign - 1) & ~(headerAlign - 1);
        
        for (usize i = 0; i < NUM_POOLS; ++i) {
            usize poolSize = MIN_POOL_SIZE << i;
            objectPools[i] = std::make_unique<ObjectPool>(poolSize + headerSize);
        }
    }

    usize GCAllocator::getPoolIndex(usize size) const {
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

    void* GCAllocator::allocateRaw(usize size, GCObjectType type, bool isGCObject) {
        if (size == 0) {
            return nullptr;
        }
    
        // Check memory limits
        if (!checkMemoryLimits(size)) {
            return nullptr;
        }
    
        void* ptr = nullptr;
    
        if (size <= MAX_POOL_SIZE) {
            ptr = allocateFromPool(size, type);
        } else {
            ptr = allocateLargeObject(size, type, isGCObject);
        }
    
        if (ptr) {
            updateStats(static_cast<isize>(size));
        
            // Check if GC should be triggered
            if (shouldTriggerGC() && gc) {
                // Note: Actual GC triggering would be handled by the caller
                // to avoid recursive allocation during GC
            }
        }
    
        return ptr;
    }

    void* GCAllocator::allocateFromPool(usize size, GCObjectType type) {
        usize poolIndex = getPoolIndex(size);
        if (poolIndex >= NUM_POOLS) {
            return nullptr;
        }
    
        return objectPools[poolIndex]->allocate(type);
    }

    void* GCAllocator::allocateLargeObject(usize size, GCObjectType type, bool isGCObject) {
        std::lock_guard<std::mutex> lock(allocatorMutex);
    
        // Calculate properly aligned header size
        constexpr usize headerAlign = alignof(MemoryBlockHeader);
        constexpr usize headerSize = (sizeof(MemoryBlockHeader) + headerAlign - 1) & ~(headerAlign - 1);
        usize totalSize = size + headerSize;
        
        void* rawPtr = _aligned_malloc(totalSize, std::max(alignof(std::max_align_t), headerAlign));
        if (!rawPtr) {
            return nullptr;
        }
    
        // Initialize header
        MemoryBlockHeader* header = new(rawPtr) 
        MemoryBlockHeader(size, type, alignof(std::max_align_t), isGCObject);
    
        // Store in large objects map
        void* userPtr = reinterpret_cast<char*>(rawPtr) + headerSize;
        largeObjects[userPtr] = header;
    
        return userPtr;
    }

    void GCAllocator::deallocate(void* ptr) {
        if (!ptr) return;
    
        // Try pools first
        bool foundInPool = false;
        for (auto& pool : objectPools) {
            if (pool && pool->owns(ptr)) {
                pool->deallocate(ptr);
                foundInPool = true;
                break;
            }
        }
    
        if (!foundInPool) {
            deallocateLargeObject(ptr);
        }
    
        // Update statistics would need the original size
        // This is a limitation of the current design
        // In practice, we'd store size information with each allocation
    }

    void GCAllocator::deallocateLargeObject(void* ptr) {
        std::lock_guard<std::mutex> lock(allocatorMutex);
    
        auto it = largeObjects.find(ptr);
        if (it != largeObjects.end()) {
            MemoryBlockHeader* header = it->second;
            usize size = header->size;
        
            // Remove from map
            largeObjects.erase(it);
        
            // Free memory
            void* rawPtr = reinterpret_cast<char*>(ptr) - sizeof(MemoryBlockHeader);
            _aligned_free(rawPtr);
        
            // Update statistics
            updateStats(-static_cast<isize>(size));
        }
    }

    void* GCAllocator::reallocate(void* ptr, usize newSize) {
        if (!ptr) {
            return allocateRaw(newSize);
        }
    
        if (newSize == 0) {
            deallocate(ptr);
            return nullptr;
        }
    
        // For simplicity, we'll allocate new memory and copy
        // A more sophisticated implementation would try to expand in place
        void* newPtr = allocateRaw(newSize);
        if (!newPtr) {
            return nullptr;
        }
    
        // We'd need to know the old size to copy properly
        // This is another limitation of the current design
        // In practice, we'd store size information with each allocation
    
        deallocate(ptr);
        return newPtr;
    }

    void GCAllocator::registerWithGC(GCObject* obj) {
        // This would be implemented when we have the GarbageCollector class
        // For now, it's a placeholder
        (void)obj; // Suppress unused parameter warning
    }

    void GCAllocator::updateStats(isize delta) {
        if (delta > 0) {
            totalAllocated.fetch_add(static_cast<usize>(delta));
            currentUsage.fetch_add(static_cast<usize>(delta));
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

    bool GCAllocator::checkMemoryLimits(usize requestedSize) const {
        usize currentMem = currentUsage.load();
        usize maxThreshold = config.maxThreshold;
    
        return (currentMem + requestedSize) <= maxThreshold;
    }

    void GCAllocator::getPoolStats(Vec<usize>& poolUsage, Vec<usize>& poolMemory) const {
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

    void GCAllocator::handleMemoryPressure() {
        // Force garbage collection if possible
        if (gc) {
            // This would trigger GC - implementation depends on GC interface
        }
    
        // Reduce GC threshold temporarily
        usize currentThreshold = gcThreshold.load();
        usize newThreshold = std::max(currentThreshold / 2, config.initialThreshold / 2);
        gcThreshold.store(newThreshold);
    }

    void GCAllocator::defragment() {
        std::lock_guard<std::mutex> lock(allocatorMutex);
    
        // For object pools, defragmentation is limited since we use fixed-size chunks
        // We could implement chunk compaction here if needed
    
        // For large objects, we could implement compaction
        // This is a complex operation that would require cooperation with the GC
    
        // For now, this is a placeholder
    }
}