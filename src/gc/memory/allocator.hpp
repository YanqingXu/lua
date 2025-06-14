#pragma once

#include "../../common/types.hpp"
#include "../utils/gc_types.hpp"
#include "../core/gc_object.hpp"
#include <mutex>
#include <array>
#include <cassert>
#include <cstdlib>

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
     * @brief Memory block header for tracking allocations
     * 
     * Each allocated block has a header that contains metadata
     * for garbage collection and memory management.
     */
    struct MemoryBlockHeader {
        usize size;                    // Size of the allocated block
        GCObjectType objectType;       // Type of the object
        u8 alignment;                  // Alignment requirement
        bool isGCObject;               // Whether this is a GC object
        MemoryBlockHeader* next;       // Next block in free list
        MemoryBlockHeader* prev;       // Previous block in free list
        
        MemoryBlockHeader(usize sz, GCObjectType type, u8 align, bool gcObj)
            : size(sz), objectType(type), alignment(align), isGCObject(gcObj)
            , next(nullptr), prev(nullptr) {}
    };

    /**
     * @brief Object pool for efficient allocation of same-sized objects
     * 
     * Maintains a free list of objects of a specific size to reduce
     * allocation overhead and memory fragmentation.
     */
    class ObjectPool {
    private:
        usize objectSize;              // Size of objects in this pool
        usize chunkSize;               // Size of each memory chunk
        usize objectsPerChunk;         // Number of objects per chunk
        Vec<void*> chunks;             // Memory chunks
        MemoryBlockHeader* freeList;   // Free object list
        usize totalObjects;            // Total objects allocated
        usize freeObjects;             // Free objects available
        mutable std::mutex poolMutex;  // Thread safety
        
    public:
        explicit ObjectPool(usize objSize, usize chunkSz = 64 * 1024)
            : objectSize(objSize), chunkSize(std::max(chunkSz, objSize))
            , objectsPerChunk(std::max(chunkSz, objSize) / objSize), freeList(nullptr)
            , totalObjects(0), freeObjects(0) {
            lua_assert(objSize >= sizeof(MemoryBlockHeader));
        lua_assert(this->chunkSize >= objSize);
        }
        
        ~ObjectPool() {
            for (void* chunk : chunks) {
                _aligned_free(chunk);
            }
        }
        
        // Allocate an object from the pool
        void* allocate(GCObjectType type);
        
        // Deallocate an object back to the pool
        void deallocate(void* ptr);
        
        // Check if pointer belongs to this pool
        bool owns(void* ptr) const;
        
        // Get pool statistics
        usize getTotalObjects() const { return totalObjects; }
        usize getFreeObjects() const { return freeObjects; }
        usize getUsedObjects() const { return totalObjects - freeObjects; }
        usize getMemoryUsage() const { return chunks.size() * chunkSize; }
        
    private:
        // Allocate a new chunk of memory
        void allocateChunk();
        
        // Initialize free list in a chunk
        void initializeFreeList(void* chunk);
    };

    /**
     * @brief GC-aware memory allocator
     * 
     * Provides efficient memory allocation for garbage-collected objects
     * with support for object pools, memory tracking, and integration
     * with the garbage collector.
     */
    class GCAllocator {
    private:
        // Object pools for common sizes (powers of 2)
        static constexpr usize NUM_POOLS = 16;
        static constexpr usize MIN_POOL_SIZE = 16;
        static constexpr usize MAX_POOL_SIZE = MIN_POOL_SIZE << (NUM_POOLS - 1);
        
        std::array<UPtr<ObjectPool>, NUM_POOLS> objectPools;
        
        // Large object allocation (> MAX_POOL_SIZE)
        HashMap<void*, MemoryBlockHeader*> largeObjects;
        
        // Memory statistics
        GCStats* stats;                // Reference to GC statistics
        Atom<usize> totalAllocated{0}; // Total memory allocated
        Atom<usize> totalFreed{0};     // Total memory freed
        Atom<usize> currentUsage{0};   // Current memory usage
        Atom<usize> gcThreshold;       // GC trigger threshold
        
        // GC integration
        GarbageCollector* gc;          // Reference to garbage collector
        State* luaState;               // Reference to Lua state
        
        // Thread safety
        mutable std::mutex allocatorMutex;
        
        // Configuration
        GCConfig config;
        
    public:
        explicit GCAllocator(GCConfig cfg = GCConfig{})
            : stats(nullptr), gcThreshold(cfg.initialThreshold)
            , gc(nullptr), luaState(nullptr), config(cfg) {
            initializeObjectPools();
        }
        
        ~GCAllocator() = default;
        
        // Disable copy and assignment
        GCAllocator(const GCAllocator&) = delete;
        GCAllocator& operator=(const GCAllocator&) = delete;
        
        // Allow move
        GCAllocator(GCAllocator&&) = default;
        GCAllocator& operator=(GCAllocator&&) = default;
        
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
        
        // Get pool statistics
        void getPoolStats(Vec<usize>& poolUsage, Vec<usize>& poolMemory) const;
        
        // Configuration
        const GCConfig& getConfig() const { return config; }
        void updateConfig(const GCConfig& newConfig) {
            ScopedLock lock(allocatorMutex);
            config = newConfig;
            gcThreshold.store(config.initialThreshold);
        }
        
        // Memory pressure handling
        void handleMemoryPressure();
        
        // Cleanup and defragmentation
        void defragment();
        
    private:
        // Initialize object pools
        void initializeObjectPools();
        
        // Get pool index for size
        usize getPoolIndex(usize size) const;
        
        // Allocate from appropriate pool
        void* allocateFromPool(usize size, GCObjectType type);
        
        // Allocate large object
        void* allocateLargeObject(usize size, GCObjectType type, bool isGCObject);
        
        // Deallocate large object
        void deallocateLargeObject(void* ptr);
        
        // Register object with GC
        void registerWithGC(GCObject* obj);
        
        // Update memory statistics
        void updateStats(isize delta);
        
        // Check memory limits
        bool checkMemoryLimits(usize requestedSize) const;
    };

    /**
     * @brief RAII wrapper for GC allocations
     * 
     * Provides automatic cleanup and exception safety for GC allocations.
     */
    template<typename T>
    class GCPtr {
    private:
        T* ptr;
        GCAllocator* allocator;
        
    public:
        explicit GCPtr(T* p = nullptr, GCAllocator* alloc = nullptr)
            : ptr(p), allocator(alloc) {}
        
        ~GCPtr() {
            if (ptr && allocator) {
                allocator->deallocate(ptr);
            }
        }
        
        // Disable copy
        GCPtr(const GCPtr&) = delete;
        GCPtr& operator=(const GCPtr&) = delete;
        
        // Allow move
        GCPtr(GCPtr&& other) noexcept
            : ptr(other.ptr), allocator(other.allocator) {
            other.ptr = nullptr;
            other.allocator = nullptr;
        }
        
        GCPtr& operator=(GCPtr&& other) noexcept {
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

    // Utility functions for creating GC objects
    template<typename T, typename... Args>
    GCPtr<T> make_gc_object(GCAllocator& allocator, GCObjectType type, Args&&... args) {
        T* obj = allocator.allocateObject<T>(type, std::forward<Args>(args)...);
        return GCPtr<T>(obj, &allocator);
    }

    // Global allocator instance (optional)
    extern GCAllocator* g_gcAllocator;
    
    // Convenience functions
    inline void setGlobalAllocator(GCAllocator* allocator) {
        g_gcAllocator = allocator;
    }
    
    inline GCAllocator* getGlobalAllocator() {
        return g_gcAllocator;
    }
}