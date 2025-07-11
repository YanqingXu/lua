#include "userdata.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/core/gc_ref.hpp"
#include <cstring>
#include <stdexcept>
#include <cstdlib>

namespace Lua {
    
    // === Static Factory Methods ===
    
    GCRef<Userdata> Userdata::createLight(void* ptr) {
        if (!ptr) {
            throw std::invalid_argument("Light userdata cannot wrap null pointer");
        }

        // Light userdata is not managed by GC, but we still need to allocate
        // the Userdata object itself through the GC system
        auto* ud = new Userdata(ptr);
        return GCRef<Userdata>(ud);
    }

    GCRef<Userdata> Userdata::createFull(usize size) {
        if (size == 0) {
            throw std::invalid_argument("Full userdata size cannot be zero");
        }

        // Allocate userdata object with additional space for user data
        // For now, we'll use a simple approach and allocate separately
        auto* ud = new Userdata(size);
        return GCRef<Userdata>(ud);
    }
    
    // === Constructors ===
    
    Userdata::Userdata(void* ptr) 
        : GCObject(GCObjectType::Userdata, sizeof(Userdata))
        , type_(UserdataType::Light)
        , size_(0)  // Light userdata doesn't track size
        , data_(ptr)
        , metatable_(nullptr) {
        // Light userdata cannot have metatables in Lua 5.1
    }
    
    Userdata::Userdata(usize size)
        : GCObject(GCObjectType::Userdata, sizeof(Userdata) + size)
        , type_(UserdataType::Full)
        , size_(size)
        , data_(nullptr)
        , metatable_(nullptr) {
        
        initializeFullUserdata();
    }

    Userdata::~Userdata() {
        // Free allocated memory for full userdata
        if (type_ == UserdataType::Full && data_) {
            std::free(data_);
            data_ = nullptr;
        }
    }
    
    // === Public Interface ===
    
    GCRef<Table> Userdata::getMetatable() const {
        if (type_ == UserdataType::Light) {
            return GCRef<Table>(nullptr);  // Light userdata has no metatable
        }
        return metatable_;
    }
    
    void Userdata::setMetatable(GCRef<Table> mt) {
        if (type_ == UserdataType::Light) {
            throw std::runtime_error("Cannot set metatable on light userdata");
        }
        metatable_ = mt;
    }
    
    bool Userdata::hasMetatable() const {
        return type_ == UserdataType::Full && metatable_;
    }
    
    // === GCObject Interface Implementation ===
    
    void Userdata::markReferences(GarbageCollector* gc) {
        // Mark metatable if present (only for full userdata)
        if (type_ == UserdataType::Full && metatable_) {
            // Use reinterpret_cast since Table inherits from GCObject
            // but we can't include table.hpp here due to circular dependency
            gc->markObject(reinterpret_cast<GCObject*>(metatable_.get()));
        }

        // Note: We don't mark the user data contents because we don't know
        // if they contain GC references. If userdata contains GC objects,
        // the user should provide a custom marking function through metatables
        // or by subclassing Userdata and overriding this method.
    }
    
    usize Userdata::getSize() const {
        if (type_ == UserdataType::Light) {
            return sizeof(Userdata);  // Only the object itself
        } else {
            return sizeof(Userdata) + size_;  // Object + user data
        }
    }
    
    usize Userdata::getAdditionalSize() const {
        return (type_ == UserdataType::Full) ? size_ : 0;
    }
    
    void Userdata::finalize() {
        // Default finalization does nothing
        // Subclasses can override this for custom cleanup
        
        // Clear metatable reference
        if (type_ == UserdataType::Full) {
            metatable_ = GCRef<Table>(nullptr);
        }
    }
    
    // === Private Implementation ===
    
    void Userdata::initializeFullUserdata() {
        // For now, use a simple approach: allocate data separately
        // In a production implementation, this would be optimized to allocate
        // the userdata and its data in a single block

        data_ = std::malloc(size_);
        if (!data_) {
            throw std::bad_alloc();
        }

        // Zero-initialize the user data
        std::memset(data_, 0, size_);
    }
}
