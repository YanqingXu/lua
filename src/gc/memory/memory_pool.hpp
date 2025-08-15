#pragma once

#include "../../common/types.hpp"
#include "../utils/gc_types.hpp"
#include "../core/gc_object.hpp"
#include <mutex>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <memory>
#include <vector>
#include <cstddef>
#include <type_traits>

// Define custom assert to avoid macro conflicts
#ifdef assert
#undef assert
#endif
#define lua_assert(expr) ((void)((expr) || (std::abort(), 0)))

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class State;

    /**
     * @brief Memory chunk for pool allocation
     * 
     * Represents a contiguous block of memory that can be divided
     * into smaller objects of the same size.
     */
    struct MemoryChunk {
        void* memory;           // Pointer to the memory block
        usize size;             // Total size of the chunk
        usize objectSize;       // Size of each object in this chunk
        usize objectCount;      // Number of objects in this chunk
        usize freeCount;        // Number of free objects
        void* freeList;         // Head of free list
        MemoryChunk* next;      // Next chunk in the pool
        
        MemoryChunk(usize chunkSize, usize objSize)
            : memory(nullptr), size(chunkSize), objectSize(objSize)
            , objectCount(chunkSize / objSize), freeCount(0)
            , freeList(nullptr), next(nullptr) {
            lua_assert(objSize > 0 && chunkSize >= objSize);
        }
        
        ~MemoryChunk() {
            if (memory) {
                _aligned_free(memory);
            }
        }
        
        // Initialize the chunk and set up free list
        bool initialize();
        
        // Allocate an object from this chunk
        void* allocate();
        
        // Deallocate an object back to this chunk
        void deallocate(void* ptr);
        
        // Check if pointer belongs to this chunk
        bool owns(void* ptr) const;
        
        // Check if chunk is full
        bool isFull() const { return freeCount == 0; }
        
        // Check if chunk is empty
        bool isEmpty() const { return freeCount == objectCount; }
        
        // Get memory usage
        usize getMemoryUsage() const { return size; }
        usize getUsedMemory() const { return (objectCount - freeCount) * objectSize; }
    };

    /**
     * @brief Fixed-size memory pool
     * 
     * Manages allocation of objects of a specific size using
     * a linked list of memory chunks.
     */
    class FixedSizePool {
    private:
        usize objectSize;           // Size of objects in this pool
        usize chunkSize;            // Size of each memory chunk
        usize maxChunks;            // Maximum number of chunks
        MemoryChunk* chunks;        // Head of chunk list
        MemoryChunk* currentChunk;  // Current chunk for allocation
        usize totalChunks;          // Total number of chunks
        usize totalObjects;         // Total objects allocated
        usize freeObjects;          // Total free objects
        mutable std::mutex poolMutex; // Thread safety
        
        // Statistics
        usize allocCount;           // Number of allocations
        usize deallocCount;         // Number of deallocations
        usize chunkAllocCount;      // Number of chunk allocations
        
    public:
        explicit FixedSizePool(usize objSize, usize chunkSz = 64 * 1024, usize maxChunks = 1024)
            : objectSize(objSize), chunkSize(chunkSz), maxChunks(maxChunks)
            , chunks(nullptr), currentChunk(nullptr), totalChunks(0)
            , totalObjects(0), freeObjects(0), allocCount(0)
            , deallocCount(0), chunkAllocCount(0) {
            lua_assert(objSize > 0);
            lua_assert(chunkSz >= objSize);
        }
        
        ~FixedSizePool() {
            cleanup();
        }
        
        // Disable copy and assignment
        FixedSizePool(const FixedSizePool&) = delete;
        FixedSizePool& operator=(const FixedSizePool&) = delete;
        
        // Allow move
        FixedSizePool(FixedSizePool&& other) noexcept;
        FixedSizePool& operator=(FixedSizePool&& other) noexcept;
        
        // Allocate an object from the pool
        void* allocate();
        
        // Deallocate an object back to the pool
        void deallocate(void* ptr);
        
        // Check if pointer belongs to this pool
        bool owns(void* ptr) const;
        
        // Get pool statistics
        usize getObjectSize() const { return objectSize; }
        usize getTotalChunks() const { return totalChunks; }
        usize getTotalObjects() const { return totalObjects; }
        usize getFreeObjects() const { return freeObjects; }
        usize getUsedObjects() const { return totalObjects - freeObjects; }
        usize getMemoryUsage() const { return totalChunks * chunkSize; }
        usize getUsedMemory() const { return getUsedObjects() * objectSize; }
        usize getAllocCount() const { return allocCount; }
        usize getDeallocCount() const { return deallocCount; }
        usize getChunkAllocCount() const { return chunkAllocCount; }
        
        // Memory management
        void shrink();              // Remove empty chunks
        void cleanup();             // Clean up all chunks
        bool canShrink() const;     // Check if pool can be shrunk
        
        // Configuration
        void setMaxChunks(usize max) { maxChunks = max; }
        usize getMaxChunks() const { return maxChunks; }
        
    private:
        // Allocate a new chunk
        MemoryChunk* allocateChunk();
        
        // Find chunk that owns the pointer
        MemoryChunk* findOwningChunk(void* ptr) const;
        
        // Remove empty chunks from the list
        void removeEmptyChunks();
    };

    /**
     * @brief Multi-size memory pool manager
     * 
     * Manages multiple fixed-size pools to handle objects of different sizes.
     * Uses a size-class approach similar to tcmalloc.
     */
    class MemoryPoolManager {
    private:
        // Size classes (powers of 2 up to a limit, then linear)
        static constexpr usize MIN_SIZE_CLASS = 8;
        static constexpr usize MAX_SMALL_SIZE = 1024;
        static constexpr usize MAX_MEDIUM_SIZE = 32 * 1024;
        static constexpr usize LARGE_SIZE_THRESHOLD = 64 * 1024;
        
        // Pool configuration
        struct PoolConfig {
            usize chunkSize;
            usize maxChunks;
            
            PoolConfig(usize cs = 64 * 1024, usize mc = 1024)
                : chunkSize(cs), maxChunks(mc) {}
        };
        
        // Size class mapping
        Vec<usize> sizeClasses;                    // Size class values
        Vec<UPtr<FixedSizePool>> pools;           // Pools for each size class
        HashMap<usize, usize> sizeToClassMap;     // Size to class index mapping
        
        // Large object allocation (direct malloc)
        HashMap<void*, usize> largeObjects;       // Large object tracking
        
        // Thread safety
        mutable std::mutex managerMutex;
        
        // Statistics
        Atom<usize> totalAllocated{0};
        Atom<usize> totalFreed{0};
        Atom<usize> currentUsage{0};
        Atom<usize> smallAllocCount{0};
        Atom<usize> mediumAllocCount{0};
        Atom<usize> largeAllocCount{0};
        
        // Configuration
        PoolConfig defaultConfig;
        
    public:
        explicit MemoryPoolManager(const PoolConfig& config = PoolConfig{})
            : defaultConfig(config) {
            initializeSizeClasses();
            initializePools();
        }
        
        ~MemoryPoolManager() {
            cleanup();
        }
        
        // Disable copy and assignment
        MemoryPoolManager(const MemoryPoolManager&) = delete;
        MemoryPoolManager& operator=(const MemoryPoolManager&) = delete;
        
        // Allow move
        MemoryPoolManager(MemoryPoolManager&&) = default;
        MemoryPoolManager& operator=(MemoryPoolManager&&) = default;
        
        // Allocate memory of specified size
        void* allocate(usize size);
        
        // Deallocate memory
        void deallocate(void* ptr, usize size = 0);
        
        // Reallocate memory
        void* reallocate(void* ptr, usize oldSize, usize newSize);
        
        // Check if pointer was allocated by this manager
        bool owns(void* ptr) const;
        
        // Get allocation size class for given size
        usize getSizeClass(usize size) const;
        
        // Get actual allocated size for given size
        usize getAllocatedSize(usize size) const;
        
        // Memory management
        void shrinkPools();         // Shrink all pools
        void cleanup();             // Clean up all pools
        void defragment();          // Defragment memory
        
        // Statistics
        usize getTotalAllocated() const { return totalAllocated.load(); }
        usize getTotalFreed() const { return totalFreed.load(); }
        usize getCurrentUsage() const { return currentUsage.load(); }
        usize getSmallAllocCount() const { return smallAllocCount.load(); }
        usize getMediumAllocCount() const { return mediumAllocCount.load(); }
        usize getLargeAllocCount() const { return largeAllocCount.load(); }
        
        // Pool statistics
        void getPoolStats(Vec<usize>& sizes, Vec<usize>& usage, Vec<usize>& memory) const;
        
        // Configuration
        void setPoolConfig(usize sizeClass, const PoolConfig& config);
        PoolConfig getPoolConfig(usize sizeClass) const;
        
    private:
        // Initialize size classes
        void initializeSizeClasses();
        
        // Initialize pools for each size class
        void initializePools();
        
        // Get pool index for size
        usize getPoolIndex(usize size) const;
        
        // Allocate large object (> LARGE_SIZE_THRESHOLD)
        void* allocateLarge(usize size);
        
        // Deallocate large object
        void deallocateLarge(void* ptr);
        
        // Update statistics
        void updateStats(isize delta, bool isLarge = false);
        
        // Round up to next size class
        usize roundUpToSizeClass(usize size) const;
    };

    /**
     * @brief GC-aware memory pool
     * 
     * Integrates memory pool management with garbage collection,
     * providing efficient allocation for GC objects.
     */
    class GCMemoryPool {
    private:
        MemoryPoolManager poolManager;      // Underlying pool manager
        GarbageCollector* gc;               // Reference to GC
        State* luaState;                    // Reference to Lua state
        GCStats* stats;                     // GC statistics
        
        // GC integration
        Atom<usize> gcThreshold;            // GC trigger threshold
        GCConfig config;                    // GC configuration
        
        // Object tracking for GC
        HashMap<void*, GCObjectType> gcObjects; // GC object type mapping
        mutable std::mutex gcMutex;         // GC object mutex
        
    public:
        explicit GCMemoryPool(const GCConfig& cfg = GCConfig{})
            : gc(nullptr), luaState(nullptr), stats(nullptr)
            , gcThreshold(cfg.initialThreshold), config(cfg) {}
        
        ~GCMemoryPool() = default;
        
        // Initialize with GC components
        void initialize(GarbageCollector* collector, State* state, GCStats* statistics) {
            gc = collector;
            luaState = state;
            stats = statistics;
        }
        
        // Allocate GC object
        template<typename T, typename... Args>
        T* allocateObject(GCObjectType type, Args&&... args) {
            static_assert(std::is_base_of_v<GCObject, T>, "T must inherit from GCObject");
            
            void* ptr = allocateRaw(sizeof(T), type);
            if (!ptr) {
                return nullptr;
            }
            
            // Construct object in-place
            T* obj = new(ptr) T(std::forward<Args>(args)...);
            
            // Register with GC
            registerWithGC(obj, type);
            
            return obj;
        }
        
        // Allocate raw memory
        void* allocateRaw(usize size, GCObjectType type = GCObjectType::String);
        
        // Deallocate memory
        void deallocate(void* ptr);
        
        // Reallocate memory
        void* reallocate(void* ptr, usize newSize);
        
        // GC integration
        bool shouldTriggerGC() const {
            return poolManager.getCurrentUsage() >= gcThreshold.load();
        }
        
        void updateGCThreshold(usize newThreshold) {
            gcThreshold.store(newThreshold);
        }
        
        // Statistics
        usize getCurrentUsage() const { return poolManager.getCurrentUsage(); }
        usize getGCThreshold() const { return gcThreshold.load(); }
        
        // Memory management
        void handleMemoryPressure();
        void defragment() { poolManager.defragment(); }
        
        // Access to underlying pool manager
        MemoryPoolManager& getPoolManager() { return poolManager; }
        const MemoryPoolManager& getPoolManager() const { return poolManager; }
        
    private:
        // Register object with GC
        void registerWithGC(GCObject* obj, GCObjectType type);
        
        // Update GC statistics
        void updateGCStats(isize delta);
    };

    // Global memory pool instance
    extern GCMemoryPool* g_memoryPool;
    
    // Convenience functions
    inline void setGlobalMemoryPool(GCMemoryPool* pool) {
        g_memoryPool = pool;
    }
    
    inline GCMemoryPool* getGlobalMemoryPool() {
        return g_memoryPool;
    }
}