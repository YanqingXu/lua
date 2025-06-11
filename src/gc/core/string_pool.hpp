#pragma once

#include "../../common/types.hpp"
#include "gc_string.hpp"
#include <unordered_set>
#include <mutex>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    
    /**
     * @brief String pool for string interning
     * 
     * This class implements string interning to reduce memory usage by ensuring
     * that identical strings share the same memory location. It maintains a pool
     * of all created GCString objects and returns existing instances when possible.
     * 
     * The string pool is integrated with the garbage collector to ensure proper
     * memory management of interned strings.
     */
    class StringPool {
    private:
        // Hash set to store unique GCString objects
        std::unordered_set<GCString*, GCStringHash, GCStringEqual> pool;
        
        // Mutex for thread safety
        mutable std::mutex poolMutex;
        
        // Private constructor for singleton pattern
        StringPool() = default;
        
    public:
        // Disable copy and move operations
        StringPool(const StringPool&) = delete;
        StringPool& operator=(const StringPool&) = delete;
        StringPool(StringPool&&) = delete;
        StringPool& operator=(StringPool&&) = delete;
        
        /**
         * @brief Get the singleton instance of the string pool
         * @return Reference to the string pool instance
         */
        static StringPool& getInstance();
        
        /**
         * @brief Intern a string (create or return existing)
         * @param str The string content
         * @return Pointer to the interned GCString
         */
        GCString* intern(const Str& str);
        
        /**
         * @brief Intern a C-style string (create or return existing)
         * @param cstr The C-style string
         * @return Pointer to the interned GCString
         */
        GCString* intern(const char* cstr);
        
        /**
         * @brief Intern a string with move semantics (create or return existing)
         * @param str The string content to move
         * @return Pointer to the interned GCString
         */
        GCString* intern(Str&& str);
        
        /**
         * @brief Remove a string from the pool
         * 
         * This method is called by the garbage collector when a string
         * is being collected to remove it from the pool.
         * 
         * @param gcString The string to remove
         */
        void remove(GCString* gcString);
        
        /**
         * @brief Mark all strings in the pool as reachable
         * 
         * This method is called during the mark phase of garbage collection
         * to mark all strings in the pool as reachable from roots.
         * 
         * @param gc Pointer to the garbage collector
         */
        void markAll(GarbageCollector* gc);
        
        /**
         * @brief Get the number of strings in the pool
         * @return Number of interned strings
         */
        usize size() const;
        
        /**
         * @brief Check if the pool is empty
         * @return true if the pool is empty
         */
        bool empty() const;
        
        /**
         * @brief Clear all strings from the pool
         * 
         * This method should only be called during shutdown or
         * when all strings are known to be unreachable.
         */
        void clear();
        
        /**
         * @brief Get memory usage statistics
         * @return Total memory used by the string pool in bytes
         */
        usize getMemoryUsage() const;
        
        /**
         * @brief Get all strings in the pool for GC marking
         * @return Vector containing all GCString pointers in the pool
         */
        std::vector<GCString*> getAllStrings() const;
    };
}