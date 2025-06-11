#pragma once

#include <type_traits>
#include "../../common/types.hpp"
#include "../memory/allocator.hpp"
#include "gc_object.hpp"
#include "gc_string.hpp"

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class GCAllocator;
    class Table;
    class Function;
    
    // Include complete type definitions
    class Table;
    class Function;

    /**
     * @brief Lightweight reference to a garbage-collected object
     * 
     * GCRef provides a type-safe way to reference GC objects without
     * interfering with the garbage collection process. Unlike smart pointers,
     * GCRef does not manage object lifetime - that's handled by the GC.
     * 
     * Key features:
     * - Zero overhead: just a typed pointer wrapper
     * - Type safety: prevents incorrect casts
     * - GC integration: works seamlessly with mark-and-sweep
     * - Null safety: provides safe null checking
     */
    template<typename T>
    class GCRef {
        // static_assert(std::is_base_of<GCObject, T>::value, "T must inherit from GCObject");
        
    private:
        T* ptr;
        
    public:
        // Constructors
        constexpr GCRef() noexcept : ptr(nullptr) {}
        constexpr GCRef(std::nullptr_t) noexcept : ptr(nullptr) {}
        explicit GCRef(T* p) noexcept : ptr(p) {}
        
        // Copy and move constructors
        GCRef(const GCRef&) noexcept = default;
        GCRef(GCRef&&) noexcept = default;
        
        // Assignment operators
        GCRef& operator=(const GCRef&) noexcept = default;
        GCRef& operator=(GCRef&&) noexcept = default;
        GCRef& operator=(std::nullptr_t) noexcept {
            ptr = nullptr;
            return *this;
        }
        
        // Access operators
        T* operator->() const noexcept {
            return ptr;
        }
        
        T& operator*() const noexcept {
            return *ptr;
        }
        
        // Get raw pointer
        T* get() const noexcept {
            return ptr;
        }
        
        // Null checking
        explicit operator bool() const noexcept {
            return ptr != nullptr;
        }
        
        bool operator!() const noexcept {
            return ptr == nullptr;
        }
        
        // Comparison operators
        bool operator==(const GCRef& other) const noexcept {
            return ptr == other.ptr;
        }
        
        bool operator!=(const GCRef& other) const noexcept {
            return ptr != other.ptr;
        }
        
        bool operator==(std::nullptr_t) const noexcept {
            return ptr == nullptr;
        }
        
        bool operator!=(std::nullptr_t) const noexcept {
            return ptr != nullptr;
        }
        
        // Less than operator for containers
        bool operator<(const GCRef& other) const noexcept {
            return std::less<T*>()(ptr, other.ptr);
        }
        
        // Reset to null
        void reset() noexcept {
            ptr = nullptr;
        }
        
        // Swap with another GCRef
        void swap(GCRef& other) noexcept {
            std::swap(ptr, other.ptr);
        }
        
        // Get as GCObject pointer
        GCObject* asGCObject() const noexcept {
            return static_cast<GCObject*>(ptr);
        }
        
        // Type casting (safe downcast)
        template<typename U>
        GCRef<U> cast() const noexcept {
            static_assert(std::is_base_of_v<GCObject, U>, "U must inherit from GCObject");
            return GCRef<U>(static_cast<U*>(ptr));
        }
        
        // Dynamic cast (returns null GCRef if cast fails)
        template<typename U>
        GCRef<U> dynamicCast() const noexcept {
            static_assert(std::is_base_of_v<GCObject, U>, "U must inherit from GCObject");
            return GCRef<U>(dynamic_cast<U*>(ptr));
        }
    };
    
    // Utility functions for creating GC objects
    
    /**
     * @brief Create a new GC object using the global allocator
     */
    template<typename T, typename... Args>
    GCRef<T> make_gc_ref(GCObjectType type, Args&&... args) {
        // static_assert(std::is_base_of<GCObject, T>::value, "T must inherit from GCObject");
        
        // Use global GC allocator if available
        extern GCAllocator* g_gcAllocator;
        if (g_gcAllocator) {
            T* obj = g_gcAllocator->allocateObject<T>(type, std::forward<Args>(args)...);
            return GCRef<T>(obj);
        }
        
        // Fallback: allocate with new (not recommended for production)
        T* obj = new T(std::forward<Args>(args)...);
        return GCRef<T>(obj);
    }
    
    /**
     * @brief Create a new GC object using a specific allocator
     */
    template<typename T, typename... Args>
    GCRef<T> make_gc_ref(GCAllocator& allocator, GCObjectType type, Args&&... args) {
        // static_assert(std::is_base_of<GCObject, T>::value, "T must inherit from GCObject");
        
        T* obj = allocator.allocateObject<T>(type, std::forward<Args>(args)...);
        return GCRef<T>(obj);
    }
    
    // Specialization for common GC types
    
    /**
     * @brief Create a GC string
     */
    inline GCRef<GCString> make_gc_string(const Str& str) {
        return make_gc_ref<GCString>(GCObjectType::String, str);
    }
    
    inline GCRef<GCString> make_gc_string(const char* str) {
        return make_gc_ref<GCString>(GCObjectType::String, Str(str));
    }
    
    /**
     * @brief Create a GC table
     * Note: Implementation moved to avoid circular dependencies
     */
    GCRef<Table> make_gc_table();
    
    /**
     * @brief Create a GC function
     * Note: Implementation moved to avoid circular dependencies
     */
    GCRef<Function> make_gc_function(int functionType);
    
    // Swap function
    template<typename T>
    void swap(GCRef<T>& a, GCRef<T>& b) noexcept {
        a.swap(b);
    }
}

// Hash support for GCRef (must be in std namespace)
namespace std {
    template<typename T>
    struct hash<Lua::GCRef<T>> {
        std::size_t operator()(const Lua::GCRef<T>& ref) const noexcept {
            return std::hash<T*>()(ref.get());
        }
    };
}