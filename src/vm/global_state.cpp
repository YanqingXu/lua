#include "global_state.hpp"
#include "lua_state.hpp"
// #include "../common/string.hpp"  // Using Str from types.hpp instead
// #include "../gc/memory/allocator.hpp"  // Will be implemented later
#include <algorithm>
#include <cstring>

// Temporary implementations until proper classes are available
namespace Lua {
    // Temporary String class until proper implementation
    class String {
    public:
        String(const char* str, usize len) : data_(str, len) {}
        const char* data() const { return data_.c_str(); }
        usize length() const { return data_.length(); }
    private:
        Str data_;
    };

    // Temporary MemoryAllocator class
    class MemoryAllocator {
    public:
        void* allocate(usize size) { return std::malloc(size); }
        void deallocate(void* ptr) { std::free(ptr); }
        void* reallocate(void* ptr, usize newSize) { return std::realloc(ptr, newSize); }
    };
}

namespace Lua {
    
    // StringTable implementation
    String* GlobalState::StringTable::find(const char* str, usize len) {
        if (!str || len == 0) return nullptr;
        
        // Create temporary string for lookup
        Str key(str, len);
        auto it = strings_.find(key);
        return (it != strings_.end()) ? it->second : nullptr;
    }
    
    String* GlobalState::StringTable::create(const char* str, usize len) {
        if (!str || len == 0) return nullptr;
        
        // First check if string already exists
        String* existing = find(str, len);
        if (existing) {
            return existing;
        }
        
        // Create new string object
        String* newStr = new String(str, len);
        Str key(newStr->data(), newStr->length());
        strings_[key] = newStr;
        totalSize_ += len;
        
        return newStr;
    }
    
    void GlobalState::StringTable::markAll(GarbageCollector* gc) {
        for (auto& pair : strings_) {
            if (pair.second) {
                // TODO: Fix String to GCObject conversion
                // gc->markObject(pair.second);
            }
        }
    }
    
    void GlobalState::StringTable::clear() {
        strings_.clear();
        totalSize_ = 0;
    }
    
    // GlobalState implementation
    GlobalState::GlobalState()
        : allocator_(make_unique<MemoryAllocator>())
        , gc_(nullptr)  // TODO: Fix GC initialization - needs State* not GlobalState*
        , mainThread_(nullptr)
        , registry_(nullptr)
        , gcThreshold_(1024 * 1024)  // 1MB default threshold
        , totalBytes_(0)
    {
        // Initialize metatables to nullptr
        for (int i = 0; i < LUA_NUM_TYPES; ++i) {
            metaTables_[i] = nullptr;
        }
        
        // Create registry table
        registry_ = new Table();
        
        // Create main thread
        mainThread_ = newThread();
        
        // Initialize metatables
        initializeMetaTables_();
    }
    
    GlobalState::~GlobalState() {
        // Clean up all threads
        cleanupThreads_();
        
        // Clean up registry
        delete registry_;
        registry_ = nullptr;
        
        // Clean up metatables
        for (int i = 0; i < LUA_NUM_TYPES; ++i) {
            delete metaTables_[i];
            metaTables_[i] = nullptr;
        }
        
        // Clean up string table
        stringTable_.clear();
        
        // GC and allocator will be cleaned up automatically by unique_ptr
    }
    
    LuaState* GlobalState::newThread() {
        LuaState* L = new LuaState(this);
        allThreads_.push_back(L);
        return L;
    }
    
    void GlobalState::closeThread(LuaState* L) {
        if (!L) return;
        
        // Remove from threads list
        auto it = std::find(allThreads_.begin(), allThreads_.end(), L);
        if (it != allThreads_.end()) {
            allThreads_.erase(it);
        }
        
        // Don't delete the main thread here, it will be cleaned up in destructor
        if (L != mainThread_) {
            delete L;
        }
    }
    
    void* GlobalState::allocate(usize size) {
        if (size == 0) return nullptr;
        
        void* ptr = allocator_->allocate(size);
        if (ptr) {
            updateMemoryStats_(static_cast<isize>(size));
        }
        return ptr;
    }
    
    void GlobalState::deallocate(void* ptr) {
        if (!ptr) return;
        
        // Note: We can't easily track the size being deallocated here
        // In a full implementation, we'd need to track allocation sizes
        allocator_->deallocate(ptr);
    }
    
    void* GlobalState::reallocate(void* ptr, usize oldSize, usize newSize) {
        if (newSize == 0) {
            deallocate(ptr);
            updateMemoryStats_(-static_cast<isize>(oldSize));
            return nullptr;
        }
        
        if (!ptr) {
            return allocate(newSize);
        }
        
        void* newPtr = allocator_->reallocate(ptr, newSize);
        if (newPtr) {
            updateMemoryStats_(static_cast<isize>(newSize) - static_cast<isize>(oldSize));
        }
        return newPtr;
    }
    
    void GlobalState::collectGarbage() {
        if (gc_) {
            gc_->collectGarbage();
        }
    }
    
    void GlobalState::markObject(GCObject* obj) {
        if (gc_ && obj) {
            gc_->markObject(obj);
        }
    }
    
    bool GlobalState::shouldCollectGarbage() const {
        return totalBytes_ > gcThreshold_;
    }
    
    String* GlobalState::newString(const char* str, usize len) {
        return stringTable_.create(str, len);
    }
    
    String* GlobalState::findString(const char* str, usize len) {
        return stringTable_.find(str, len);
    }
    
    Table* GlobalState::getMetaTable(int type) const {
        if (type < 0 || type >= LUA_NUM_TYPES) {
            return nullptr;
        }
        return metaTables_[type];
    }
    
    void GlobalState::setMetaTable(int type, Table* mt) {
        if (type >= 0 && type < LUA_NUM_TYPES) {
            metaTables_[type] = mt;
        }
    }
    
    void GlobalState::initializeMetaTables_() {
        // Initialize basic metatables
        // In a full implementation, these would be set up with default metamethods
        for (int i = 0; i < LUA_NUM_TYPES; ++i) {
            metaTables_[i] = nullptr;  // Will be set later as needed
        }
    }
    
    void GlobalState::cleanupThreads_() {
        // Clean up all threads except main thread (which is cleaned up last)
        for (LuaState* L : allThreads_) {
            if (L && L != mainThread_) {
                delete L;
            }
        }
        
        // Clean up main thread
        if (mainThread_) {
            delete mainThread_;
            mainThread_ = nullptr;
        }
        
        allThreads_.clear();
    }
    
    void GlobalState::updateMemoryStats_(isize delta) {
        if (delta > 0) {
            totalBytes_ += static_cast<usize>(delta);
        } else if (static_cast<usize>(-delta) <= totalBytes_) {
            totalBytes_ -= static_cast<usize>(-delta);
        } else {
            totalBytes_ = 0;  // Prevent underflow
        }
    }

    // Global variable management implementation (Phase 1 refactoring)
    void GlobalState::setGlobal(const Str& name, const Value& value) {
        if (!registry_) {
            // Fallback: create registry if it doesn't exist
            registry_ = new Table();
        }

        // Use string as key for global variable
        Value key(name);
        registry_->set(key, value);
    }

    Value GlobalState::getGlobal(const Str& name) {
        if (!registry_) {
            return Value(); // Return nil if no registry
        }

        // Use string as key for global variable lookup
        Value key(name);
        return registry_->get(key);
    }

    bool GlobalState::hasGlobal(const Str& name) const {
        if (!registry_) {
            return false;
        }

        // Check if global variable exists by getting it and checking if it's not nil
        Value key(name);
        Value result = registry_->get(key);
        return !result.isNil();
    }

} // namespace Lua
