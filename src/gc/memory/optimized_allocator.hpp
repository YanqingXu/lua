#pragma once

#include "../../common/types.hpp"
#include "../utils/gc_types.hpp"
#include "../core/gc_object.hpp"
#include "memory_pool.hpp"
#include <array>
#include <cassert>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class State;

    /**
     * @brief Lightweight memory block header for GC objects only
     * 
     * Optimized header that only stores essential GC metadata.
     * Used only when necessary (for GC objects).
     */
    struct OptimizedMemoryHeader {
        GCObjectType objectType;       // Type of the object
        u16 flags;                     // GC flags (marked, color, etc.)
        
        OptimizedMemoryHeader(GCObjectType type, u16 gcFlags = 0)
            : objectType(type), flags(gcFlags) {}
    };

    /**
     * @brief Hybrid object pool combining efficiency and GC support
     * 
     * Uses different allocation strategies based on object type:
     * - GC objects: Use OptimizedMemoryHeader for metadata
     * - Non-GC objects: Use lightweight free list without headers
     */
    class HybridObjectPool {
    private:
        
        usize objectSize;              // Size of objects in this pool
        usize chunkSize;               // Size of each memory chunk
        usize maxChunks;               // Maximum number of chunks
        bool requiresGCHeader;         // Whether objects need GC headers
        
        // Use FixedSizePool for efficient chunk management
        UPtr<FixedSizePool> gcPool;      // Pool for GC objects (with headers)
        UPtr<FixedSizePool> fastPool;    // Pool for non-GC objects (no headers)
        
        mutable std::mutex poolMutex;  // Thread safety
        
        // Statistics
        Atom<usize> gcAllocations{0};
        Atom<usize> fastAllocations{0};
        Atom<usize> gcDeallocations{0};
        Atom<usize> fastDeallocations{0};
        
    public:
        explicit HybridObjectPool(usize objSize, usize chunkSz = 64 * 1024, usize maxChunks = 1024)
            : objectSize(objSize), chunkSize(chunkSz), maxChunks(maxChunks)
            , requiresGCHeader(false) {
            
            // Create pools with appropriate sizes
            usize gcObjectSize = objSize + sizeof(OptimizedMemoryHeader);
            gcPool = std::make_unique<FixedSizePool>(gcObjectSize, chunkSz, maxChunks);
            fastPool = std::make_unique<FixedSizePool>(objSize, chunkSz, maxChunks);
        }
        
        ~HybridObjectPool() = default;
        
        // Disable copy and assignment
        HybridObjectPool(const HybridObjectPool&) = delete;
        HybridObjectPool& operator=(const HybridObjectPool&) = delete;
        
        // Allow move
        HybridObjectPool(HybridObjectPool&&) = default;
        HybridObjectPool& operator=(HybridObjectPool&&) = default;
        
        // Allocate an object (automatically chooses strategy)
        void* allocate(GCObjectType type, bool isGCObject = true);
        
        // Deallocate an object
        void deallocate(void* ptr);
        
        // Check if pointer belongs to this pool
        bool owns(void* ptr) const;
        
        // Get object type from GC object pointer
        GCObjectType getObjectType(void* ptr) const;
        
        // Get GC flags from GC object pointer
        u16 getGCFlags(void* ptr) const;
        
        // Set GC flags for GC object
        void setGCFlags(void* ptr, u16 flags);
        
        // Pool statistics
        usize getObjectSize() const { return objectSize; }
        usize getTotalObjects() const {
            return gcPool->getTotalObjects() + fastPool->getTotalObjects();
        }
        usize getFreeObjects() const {
            return gcPool->getFreeObjects() + fastPool->getFreeObjects();
        }
        usize getUsedObjects() const {
            return gcPool->getUsedObjects() + fastPool->getUsedObjects();
        }
        usize getMemoryUsage() const {
            return gcPool->getMemoryUsage() + fastPool->getMemoryUsage();
        }
        usize getGCAllocations() const { return gcAllocations.load(); }
        usize getFastAllocations() const { return fastAllocations.load(); }
        usize getGCDeallocations() const { return gcDeallocations.load(); }
        usize getFastDeallocations() const { return fastDeallocations.load(); }
        
        // Memory management
        void shrink() {
            ScopedLock lock{poolMutex};
            gcPool->shrink();
            fastPool->shrink();
        }
        
        void cleanup() {
            ScopedLock lock{poolMutex};
            gcPool->cleanup();
            fastPool->cleanup();
        }
        
    private:
        // Get header from GC object pointer
        OptimizedMemoryHeader* getHeader(void* ptr) const;
        
        // Check if pointer is from GC pool
        bool isGCObject(void* ptr) const;
    };

    /**
     * @brief Optimized GC-aware memory allocator
     * 
     * Enhanced version of GCAllocator that combines the efficiency of
     * memory_pool with the GC integration capabilities of the original allocator.
     */
    class OptimizedGCAllocator {
    private:
        // Object pools for common sizes (powers of 2)
        static constexpr usize NUM_POOLS = 16;
        static constexpr usize MIN_POOL_SIZE = 16;
        static constexpr usize MAX_POOL_SIZE = MIN_POOL_SIZE << (NUM_POOLS - 1);
        
        std::array<UPtr<HybridObjectPool>, NUM_POOLS> objectPools;
        
        // Large object allocation using MemoryPoolManager
        UPtr<MemoryPoolManager> largeObjectManager;
        
        // Memory statistics
        GCStats* stats;                // Reference to GC statistics
        Atom<usize> totalAllocated{0}; // Total memory allocated
        Atom<usize> totalFreed{0};     // Total memory freed
        Atom<usize> currentUsage{0};   // Current memory usage
        Atom<usize> gcThreshold;       // GC trigger threshold
        
        // Performance metrics
        Atom<usize> poolHits{0};       // Allocations served by pools
        Atom<usize> poolMisses{0};     // Allocations requiring large object manager
        Atom<usize> gcObjectCount{0};  // Number of GC objects
        Atom<usize> fastObjectCount{0}; // Number of fast (non-GC) objects
        
        // GC integration
        GarbageCollector* gc;          // Reference to garbage collector
        State* luaState;               // Reference to Lua state
        
        // Thread safety
        mutable std::mutex allocatorMutex;
        
        // Configuration
        GCConfig config;
        
        // Adaptive tuning
        Atom<usize> allocationPattern{0}; // Track allocation patterns
        Atom<u64> lastTuningTime{0};      // Last time pools were tuned
        
    public:
        explicit OptimizedGCAllocator(GCConfig cfg = GCConfig{})
            : stats(nullptr), gcThreshold(cfg.initialThreshold)
            , gc(nullptr), luaState(nullptr), config(cfg) {
            initializeObjectPools();
            initializeLargeObjectManager();
        }
        
        ~OptimizedGCAllocator() = default;
        
        // Disable copy and assignment
        OptimizedGCAllocator(const OptimizedGCAllocator&) = delete;
        OptimizedGCAllocator& operator=(const OptimizedGCAllocator&) = delete;
        
        // Allow move
        OptimizedGCAllocator(OptimizedGCAllocator&&) = default;
        OptimizedGCAllocator& operator=(OptimizedGCAllocator&&) = default;
        
        // Initialize allocator with GC and state references
        void initialize(GarbageCollector* collector, State* state, GCStats* statistics) {
            gc = collector;
            luaState = state;
            stats = statistics;
        }
        
        // Allocate memory for a GC object
        template<typename T, typename... Args>
        T* allocateObject(GCObjectType type, Args&&... args) {
            static_assert(std::is_base_of_v<GCObject, T>, "T must inherit from GCObject");
            
            usize size = sizeof(T);
            void* ptr = allocateRaw(size, type, true);
            if (!ptr) {
                return nullptr;
            }
            
            // Construct object in-place
            T* obj = new(ptr) T(std::forward<Args>(args)...);
            
            // Register with GC if available
            if (gc) {
                registerWithGC(obj);
            }
            
            return obj;
        }
        
        // Allocate raw memory
        void* allocateRaw(usize size, GCObjectType type = GCObjectType::String, bool isGCObject = false);
        
        // Deallocate memory
        void deallocate(void* ptr);
        
        // Reallocate memory (for dynamic arrays, strings, etc.)
        void* reallocate(void* ptr, usize newSize);
        
        // Check if GC should be triggered
        bool shouldTriggerGC() const {
            return currentUsage.load() >= gcThreshold.load();
        }
        
        // Update GC threshold after collection
        void updateGCThreshold(usize newThreshold) {
            gcThreshold.store(newThreshold);
        }
        
        // Get memory statistics
        usize getTotalAllocated() const { return totalAllocated.load(); }
        usize getTotalFreed() const { return totalFreed.load(); }
        usize getCurrentUsage() const { return currentUsage.load(); }
        usize getGCThreshold() const { return gcThreshold.load(); }
        usize getPoolHits() const { return poolHits.load(); }
        usize getPoolMisses() const { return poolMisses.load(); }
        usize getGCObjectCount() const { return gcObjectCount.load(); }
        usize getFastObjectCount() const { return fastObjectCount.load(); }
        
        // Get detailed pool statistics
        void getPoolStats(Vec<usize>& poolUsage, Vec<usize>& poolMemory) const;
        void getDetailedStats(HashMap<Str, usize>& stats) const;
        
        // Configuration
        const GCConfig& getConfig() const { return config; }
        void updateConfig(const GCConfig& newConfig) {
            ScopedLock lock{allocatorMutex};
            config = newConfig;
            gcThreshold.store(config.initialThreshold);
        }
        
        // Memory pressure handling
        void handleMemoryPressure();
        
        // Cleanup and defragmentation
        void defragment();
        
        // Adaptive tuning
        void tunePoolSizes();
        
        // Get object type for GC objects
        GCObjectType getObjectType(void* ptr) const;
        
        // GC flag management for GC objects
        u16 getGCFlags(void* ptr) const;
        void setGCFlags(void* ptr, u16 flags);
        
    private:
        // Initialize object pools
        void initializeObjectPools();
        
        // Initialize large object manager
        void initializeLargeObjectManager();
        
        // Get pool index for size
        usize getPoolIndex(usize size) const;
        
        // Allocate from appropriate pool
        void* allocateFromPool(usize size, GCObjectType type, bool isGCObject);
        
        // Allocate large object
        void* allocateLargeObject(usize size, GCObjectType type, bool isGCObject);
        
        // Register object with GC
        void registerWithGC(GCObject* obj);
        
        // Update memory statistics
        void updateStats(isize delta, bool isGCObject);
        
        // Check memory limits
        bool checkMemoryLimits(usize requestedSize) const;
        
        // Update allocation patterns for adaptive tuning
        void updateAllocationPattern(usize size, bool isGCObject);
    };

    /**
     * @brief RAII wrapper for optimized GC allocations
     * 
     * Enhanced version with better performance tracking.
     */
    template<typename T>
    class OptimizedGCPtr {
    private:
        T* ptr;
        OptimizedGCAllocator* allocator;
        
    public:
        explicit OptimizedGCPtr(T* p = nullptr, OptimizedGCAllocator* alloc = nullptr)
            : ptr(p), allocator(alloc) {}
        
        ~OptimizedGCPtr() {
            if (ptr && allocator) {
                allocator->deallocate(ptr);
            }
        }
        
        // Disable copy
        OptimizedGCPtr(const OptimizedGCPtr&) = delete;
        OptimizedGCPtr& operator=(const OptimizedGCPtr&) = delete;
        
        // Allow move
        OptimizedGCPtr(OptimizedGCPtr&& other) noexcept
            : ptr(other.ptr), allocator(other.allocator) {
            other.ptr = nullptr;
            other.allocator = nullptr;
        }
        
        OptimizedGCPtr& operator=(OptimizedGCPtr&& other) noexcept {
            if (this != &other) {
                if (ptr && allocator) {
                    allocator->deallocate(ptr);
                }
                ptr = other.ptr;
                allocator = other.allocator;
                other.ptr = nullptr;
                other.allocator = nullptr;
            }
            return *this;
        }
        
        // Access operators
        T* operator->() const { return ptr; }
        T& operator*() const { return *ptr; }
        T* get() const { return ptr; }
        
        // Release ownership
        T* release() {
            T* result = ptr;
            ptr = nullptr;
            allocator = nullptr;
            return result;
        }
        
        // Check if valid
        explicit operator bool() const { return ptr != nullptr; }
    };

    // Utility functions for creating optimized GC objects
    template<typename T, typename... Args>
    OptimizedGCPtr<T> make_optimized_gc_object(OptimizedGCAllocator& allocator, GCObjectType type, Args&&... args) {
        T* obj = allocator.allocateObject<T>(type, std::forward<Args>(args)...);
        return OptimizedGCPtr<T>(obj, &allocator);
    }

    // Global optimized allocator instance
    extern OptimizedGCAllocator* g_optimizedGCAllocator;
    
    // Convenience functions
    inline void setOptimizedGlobalAllocator(OptimizedGCAllocator* allocator) {
        g_optimizedGCAllocator = allocator;
    }
    
    inline OptimizedGCAllocator* getOptimizedGlobalAllocator() {
        return g_optimizedGCAllocator;
    }
}