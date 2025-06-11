#include "string_pool.hpp"
#include "garbage_collector.hpp"
#include <algorithm>

namespace Lua {
    
    StringPool& StringPool::getInstance() {
        static StringPool instance;
        return instance;
    }
    
    GCString* StringPool::intern(const Str& str) {
        std::lock_guard<std::mutex> lock(poolMutex);
        
        // Create a temporary GCString for lookup
        // We use a stack-allocated object to avoid unnecessary heap allocation
        GCString temp(str);
        
        // Try to find existing string in the pool
        auto it = pool.find(&temp);
        if (it != pool.end()) {
            // Found existing string, return it
            return *it;
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
        std::lock_guard<std::mutex> lock(poolMutex);
        
        // Create a temporary GCString for lookup
        GCString temp(str); // Note: we don't move here to preserve str for potential creation
        
        // Try to find existing string in the pool
        auto it = pool.find(&temp);
        if (it != pool.end()) {
            // Found existing string, return it
            return *it;
        }
        
        // String not found, create new one with move semantics and add to pool
        GCString* newString = new GCString(std::move(str));
        pool.insert(newString);
        return newString;
    }
    
    void StringPool::remove(GCString* gcString) {
        if (!gcString) return;
        
        std::lock_guard<std::mutex> lock(poolMutex);
        pool.erase(gcString);
    }
    
    void StringPool::markAll(GarbageCollector* gc) {
        if (!gc) return;
        
        std::lock_guard<std::mutex> lock(poolMutex);
        
        // Mark all strings in the pool as reachable
        for (GCString* str : pool) {
            if (str) {
                gc->markObject(str);
            }
        }
    }
    
    usize StringPool::size() const {
        std::lock_guard<std::mutex> lock(poolMutex);
        return pool.size();
    }
    
    bool StringPool::empty() const {
        std::lock_guard<std::mutex> lock(poolMutex);
        return pool.empty();
    }
    
    void StringPool::clear() {
        std::lock_guard<std::mutex> lock(poolMutex);
        pool.clear();
    }
    
    usize StringPool::getMemoryUsage() const {
        std::lock_guard<std::mutex> lock(poolMutex);
        
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
        std::lock_guard<std::mutex> lock(poolMutex);
        
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