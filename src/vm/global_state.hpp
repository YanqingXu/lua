#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/memory/allocator.hpp"
#include "value.hpp"
#include "table.hpp"
#include <memory>
#include <vector>
#include <cstdlib>

namespace Lua {
    // Forward declarations
    class LuaState;
    class String;
    class Table;
    class GarbageCollector;
    class MemoryAllocator;
    
    /**
     * @brief Global state shared by all threads in a Lua universe
     * 
     * This class manages global resources that are shared across all Lua threads,
     * following the Lua 5.1 official design pattern. It separates global state
     * from per-thread state for proper coroutine and multi-threading support.
     */
    class GlobalState {
    private:
        // Memory management
        UPtr<MemoryAllocator> allocator_;
        UPtr<GarbageCollector> gc_;
        
        // String management (string table for interning)
        struct StringTable {
            HashMap<Str, String*> strings_;
            usize totalSize_;
            
            StringTable() : totalSize_(0) {}
            
            String* find(const char* str, usize len);
            String* create(const char* str, usize len);
            void markAll(GarbageCollector* gc);
            void clear();
        } stringTable_;
        
        // Type system - metatables for basic types
        static const int LUA_NUM_TYPES = 8;  // nil, boolean, number, string, table, function, userdata, thread
        Table* metaTables_[LUA_NUM_TYPES];
        
        // Thread management
        LuaState* mainThread_;
        Vec<LuaState*> allThreads_;
        
        // Registry table for storing global references
        Table* registry_;
        
        // GC configuration
        usize gcThreshold_;
        usize totalBytes_;
        
    public:
        /**
         * @brief Construct a new Global State object
         */
        GlobalState();
        
        /**
         * @brief Destroy the Global State object
         */
        ~GlobalState();
        
        // Thread management
        /**
         * @brief Create a new Lua thread (coroutine)
         * @return LuaState* Pointer to the new thread state
         */
        LuaState* newThread();
        
        /**
         * @brief Close and cleanup a Lua thread
         * @param L Thread to close
         */
        void closeThread(LuaState* L);
        
        /**
         * @brief Get the main thread
         * @return LuaState* Pointer to the main thread
         */
        LuaState* getMainThread() const { return mainThread_; }
        
        /**
         * @brief Get all threads
         * @return const Vec<LuaState*>& Vector of all thread pointers
         */
        const Vec<LuaState*>& getAllThreads() const { return allThreads_; }
        
        // Memory management
        /**
         * @brief Allocate memory through the global allocator
         * @param size Size in bytes to allocate
         * @return void* Pointer to allocated memory
         */
        void* allocate(usize size);
        
        /**
         * @brief Deallocate memory through the global allocator
         * @param ptr Pointer to memory to deallocate
         */
        void deallocate(void* ptr);
        
        /**
         * @brief Reallocate memory through the global allocator
         * @param ptr Existing pointer (can be nullptr)
         * @param oldSize Old size in bytes
         * @param newSize New size in bytes
         * @return void* Pointer to reallocated memory
         */
        void* reallocate(void* ptr, usize oldSize, usize newSize);
        
        // GC interface
        /**
         * @brief Trigger garbage collection
         */
        void collectGarbage();
        
        /**
         * @brief Mark an object as reachable during GC
         * @param obj Object to mark
         */
        void markObject(GCObject* obj);
        
        /**
         * @brief Check if garbage collection should be triggered
         * @return true if GC should run
         */
        bool shouldCollectGarbage() const;
        
        /**
         * @brief Get the garbage collector instance
         * @return GarbageCollector* Pointer to the GC
         */
        GarbageCollector* getGC() const { return gc_.get(); }
        
        // String management
        /**
         * @brief Create or find an interned string
         * @param str C-style string
         * @param len Length of the string
         * @return String* Pointer to the interned string
         */
        String* newString(const char* str, usize len);
        
        /**
         * @brief Find an existing interned string
         * @param str C-style string
         * @param len Length of the string
         * @return String* Pointer to the string if found, nullptr otherwise
         */
        String* findString(const char* str, usize len);
        
        // Registry access
        /**
         * @brief Get the global registry table
         * @return Table* Pointer to the registry
         */
        Table* getRegistry() const { return registry_; }
        
        // Metatable management
        /**
         * @brief Get metatable for a basic type
         * @param type Type index (0-7 for basic Lua types)
         * @return Table* Pointer to the metatable, nullptr if not set
         */
        Table* getMetaTable(int type) const;
        
        /**
         * @brief Set metatable for a basic type
         * @param type Type index (0-7 for basic Lua types)
         * @param mt Metatable to set
         */
        void setMetaTable(int type, Table* mt);
        
        // Memory statistics
        /**
         * @brief Get total allocated bytes
         * @return usize Total bytes allocated
         */
        usize getTotalBytes() const { return totalBytes_; }
        
        /**
         * @brief Get GC threshold
         * @return usize Current GC threshold in bytes
         */
        usize getGCThreshold() const { return gcThreshold_; }
        
        /**
         * @brief Set GC threshold
         * @param threshold New threshold in bytes
         */
        void setGCThreshold(usize threshold) { gcThreshold_ = threshold; }

        // Global variable management (Phase 1 refactoring)
        /**
         * @brief Set global variable
         * @param name Variable name
         * @param value Variable value
         */
        void setGlobal(const Str& name, const Value& value);

        /**
         * @brief Get global variable
         * @param name Variable name
         * @return Value Variable value (nil if not found)
         */
        Value getGlobal(const Str& name);

        /**
         * @brief Check if global variable exists
         * @param name Variable name
         * @return bool True if variable exists
         */
        bool hasGlobal(const Str& name) const;
        
    private:
        // Internal helper methods
        void initializeMetaTables_();
        void cleanupThreads_();
        void updateMemoryStats_(isize delta);
        
        // Prevent copying
        GlobalState(const GlobalState&) = delete;
        GlobalState& operator=(const GlobalState&) = delete;
    };
}
