#pragma once

#include "gc_object.hpp"
#include "../../common/types.hpp"

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class State;

    /**
     * @brief Garbage-collected string object
     * 
     * This class represents a string that is managed by the garbage collector.
     * It wraps a std::string and provides the necessary GC interface methods.
     * 
     * GCString objects are immutable once created to ensure thread safety
     * and optimize memory usage through string interning.
     */
    class GCString : public GCObject {
    private:
        Str data;  // The actual string data
        u32 hash;  // Cached hash value for fast lookups
        
        /**
         * @brief Calculate hash value for the string
         * @param str The string to hash
         * @return Hash value
         */
        static u32 calculateHash(const Str& str);
        
    public:
        /**
         * @brief Construct a new GCString
         * @param str The string content
         */
        explicit GCString(const Str& str);
        
        /**
         * @brief Construct a new GCString from C-style string
         * @param cstr The C-style string
         */
        explicit GCString(const char* cstr);
        
        /**
         * @brief Construct a new GCString with move semantics
         * @param str The string content to move
         */
        explicit GCString(Str&& str);
        
        /**
         * @brief Virtual destructor
         * 
         * Removes this string from the string pool when destroyed.
         */
        virtual ~GCString();
        
        // === GCObject interface ===
        
        /**
         * @brief Mark all objects referenced by this string
         * 
         * Strings don't reference other GC objects, so this is a no-op.
         * 
         * @param gc Pointer to the garbage collector
         */
        void markReferences(GarbageCollector* gc) override;
        
        /**
         * @brief Get the size of this string object
         * @return Size in bytes including the string data
         */
        usize getSize() const override;
        
        /**
         * @brief Get additional memory used by the string data
         * @return Size of the string data in bytes
         */
        usize getAdditionalSize() const override;
        
        // === String interface ===
        
        /**
         * @brief Get the string content
         * @return Reference to the string data
         */
        const Str& getString() const { return data; }
        
        /**
         * @brief Get the length of the string
         * @return String length
         */
        usize length() const { return data.length(); }
        
        /**
         * @brief Check if the string is empty
         * @return true if empty
         */
        bool empty() const { return data.empty(); }
        
        /**
         * @brief Get the cached hash value
         * @return Hash value
         */
        u32 getHash() const { return hash; }
        
        /**
         * @brief Get C-style string
         * @return C-style string pointer
         */
        const char* c_str() const { return data.c_str(); }
        
        // === Comparison operators ===
        
        /**
         * @brief Compare with another GCString
         * @param other The other string
         * @return true if equal
         */
        bool operator==(const GCString& other) const;
        
        /**
         * @brief Compare with a regular string
         * @param str The string to compare with
         * @return true if equal
         */
        bool operator==(const Str& str) const;
        
        /**
         * @brief Less than comparison for ordering
         * @param other The other string
         * @return true if this string is less than other
         */
        bool operator<(const GCString& other) const;
        
        // === Static factory methods ===
        
        /**
         * @brief Create a new GCString
         * @param str The string content
         * @return Pointer to the new GCString
         */
        static GCString* create(const Str& str);
        
        /**
         * @brief Create a new GCString from C-style string
         * @param cstr The C-style string
         * @return Pointer to the new GCString
         */
        static GCString* create(const char* cstr);
        
        /**
         * @brief Create a new GCString with move semantics
         * @param str The string content to move
         * @return Pointer to the new GCString
         */
        static GCString* create(Str&& str);
    };
    
    // === Hash function for use in containers ===
    
    struct GCStringHash {
        usize operator()(const GCString* str) const {
            return str ? str->getHash() : 0;
        }
    };
    
    struct GCStringEqual {
        bool operator()(const GCString* a, const GCString* b) const {
            if (a == b) return true;
            if (!a || !b) return false;
            return *a == *b;
        }
    };
}