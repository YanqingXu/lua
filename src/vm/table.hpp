#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include <vector>
#include <memory>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class State;
    class Value;
    
    // Simplified Table implementation
    class Table : public GCObject {
    private:
        // Forward declaration of Entry
        struct Entry;

        // Array part
        Vec<Value> array;

        // Hash table part implemented with simple vector
        Vec<void*> entries;
        
        // Metatable for this table
        Table* metatable = nullptr;
        
    public:
        Table() : GCObject(GCObjectType::Table, sizeof(Table)) {}
        ~Table();
        
        // Override GCObject virtual functions
        void markReferences(GarbageCollector* gc) override;
        usize getSize() const override;
        usize getAdditionalSize() const override;
        
        // Get value from table
        Value get(const Value& key);
        
        // Set value in table
        void set(const Value& key, const Value& value);
        
        // Get table length
        size_t length() const;
        
        // Get array size for GC marking
        usize getArraySize() const;

        // Get array element for GC marking
        const Value& getArrayElement(usize index) const;
        
        // Iterate over hash entries for GC marking
        template<typename Func>
        void forEachHashEntry(Func&& func) const;
        
        // Get metatable
        Table* getMetatable() const { return metatable; }
        
        // Set metatable
        void setMetatable(Table* mt) { metatable = mt; }
        
        // Clear weak references (for garbage collection)
        void clearWeakReferences();
        
    private:
        // Find key in entries
        int findEntry(const Value& key) const;
    };
}
