#include "string_pool.hpp"
#include "garbage_collector.hpp"
#include "../../api/lua51_gc_api.hpp"
#include <algorithm>
#include <iostream>

namespace Lua {
    
    StringPool& StringPool::getInstance() {
        static StringPool instance;
        return instance;
    }
    
    GCString* StringPool::intern(const Str& str) {
        ScopedLock lock(poolMutex);
        
        // Search for existing string by comparing string content directly
        // to avoid creating temporary GCString objects that would call destructors
        for (auto it = pool.begin(); it != pool.end(); ++it) {
            if ((*it)->getString() == str) {
                // Found existing string, return it
                return *it;
            }
        }
        
        // String not found, create new one and add to pool
        GCString* newString = new GCString(str);
        pool.insert(newString);
        return newString;
    }
    
    GCString* StringPool::intern(const char* cstr) {
        if (!cstr) {
            cstr = ""; // Handle null pointer
        }
        return intern(Str(cstr));
    }
    
    GCString* StringPool::intern(Str&& str) {
        ScopedLock lock(poolMutex);
        
        // Search for existing string by comparing string content directly
        // to avoid creating temporary GCString objects that would call destructors
        for (auto it = pool.begin(); it != pool.end(); ++it) {
            if ((*it)->getString() == str) {
                // Found existing string, return it
                return *it;
            }
        }
        
        // String not found, create new one with move semantics and add to pool
        GCString* newString = new GCString(std::move(str));
        pool.insert(newString);
        return newString;
    }
    
    void StringPool::remove(GCString* gcString) {
        if (!gcString) return;
        
        ScopedLock lock(poolMutex);
        pool.erase(gcString);
    }
    
    void StringPool::markAll(GarbageCollector* gc) {
        if (!gc) return;
        
        ScopedLock lock(poolMutex);
        
        // Mark all strings in the pool as reachable
        for (GCString* str : pool) {
            if (str) {
                gc->markObject(str);
            }
        }
    }
    
    usize StringPool::size() const {
        ScopedLock lock(poolMutex);
        return pool.size();
    }
    
    bool StringPool::empty() const {
        ScopedLock lock(poolMutex);
        return pool.empty();
    }
    
    void StringPool::clear() {
        ScopedLock lock(poolMutex);
        pool.clear();
    }
    
    usize StringPool::getMemoryUsage() const {
        ScopedLock lock(poolMutex);
        
        usize totalSize = 0;
        
        // Calculate memory used by the pool structure itself
        totalSize += sizeof(StringPool);
        totalSize += pool.bucket_count() * sizeof(void*); // Hash table buckets
        
        // Calculate memory used by all strings in the pool
        for (const GCString* str : pool) {
            if (str) {
                totalSize += str->getSize();
                totalSize += str->getAdditionalSize();
            }
        }
        
        return totalSize;
    }
    
    std::vector<GCString*> StringPool::getAllStrings() const {
        ScopedLock lock(poolMutex);
        
        std::vector<GCString*> result;
        result.reserve(pool.size());
        
        for (GCString* str : pool) {
            if (str) {
                result.push_back(str);
            }
        }
        
        return result;
    }
}