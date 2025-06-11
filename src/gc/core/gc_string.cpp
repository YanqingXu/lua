#include "gc_string.hpp"
#include "string_pool.hpp"
#include "garbage_collector.hpp"
#include <functional>

namespace Lua {
    
    // === Private methods ===
    
    u32 GCString::calculateHash(const Str& str) {
        // Use std::hash for string hashing
        std::hash<Str> hasher;
        return static_cast<u32>(hasher(str));
    }
    
    // === Constructors ===
    
    GCString::GCString(const Str& str)
        : GCObject(GCObjectType::String, sizeof(GCString))
        , data(str)
        , hash(calculateHash(str)) {
    }
    
    GCString::GCString(const char* cstr)
        : GCObject(GCObjectType::String, sizeof(GCString))
        , data(cstr ? cstr : "")
        , hash(calculateHash(data)) {
    }
    
    GCString::GCString(Str&& str)
        : GCObject(GCObjectType::String, sizeof(GCString))
        , data(std::move(str))
        , hash(calculateHash(data)) {
    }
    
    // === GCObject interface implementation ===
    
    void GCString::markReferences(GarbageCollector* gc) {
        // Strings don't reference other GC objects, so nothing to mark
        (void)gc; // Suppress unused parameter warning
    }
    
    usize GCString::getSize() const {
        return sizeof(GCString);
    }
    
    usize GCString::getAdditionalSize() const {
        // Return the size of the string data
        return data.capacity();
    }
    
    // === Comparison operators ===
    
    bool GCString::operator==(const GCString& other) const {
        // Fast path: same object
        if (this == &other) return true;
        
        // Fast path: different hash values
        if (hash != other.hash) return false;
        
        // Compare actual string content
        return data == other.data;
    }
    
    bool GCString::operator==(const Str& str) const {
        return data == str;
    }
    
    bool GCString::operator<(const GCString& other) const {
        return data < other.data;
    }
    
    // === Destructor ===
    
    GCString::~GCString() {
        // Remove this string from the string pool when destroyed
        StringPool::getInstance().remove(this);
    }
    
    // === Static factory methods ===
    
    GCString* GCString::create(const Str& str) {
        // Use string pool for interning
        return StringPool::getInstance().intern(str);
    }
    
    GCString* GCString::create(const char* cstr) {
        // Use string pool for interning
        return StringPool::getInstance().intern(cstr);
    }
    
    GCString* GCString::create(Str&& str) {
        // Use string pool for interning
        return StringPool::getInstance().intern(std::move(str));
    }
    
}