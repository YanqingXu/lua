#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "../gc/core/gc_ref.hpp"
#include <vector>
#include <memory>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class LuaState;
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
        GCRef<Table> metatable;
        
    public:
        Table() : GCObject(GCObjectType::Table, sizeof(Table)), metatable(nullptr) {}
        ~Table();
        
        // Override GCObject virtual functions
        void markReferences(GarbageCollector* gc) override;
        usize getSize() const override;
        usize getAdditionalSize() const override;
        
        // Get value from table
        Value get(const Value& key);
        Value get(const Value& key) const;

        // Metamethod-aware operations
        Value getWithMetamethod(const Value& key, class LuaState* state = nullptr);

        // Set value in table
        void set(const Value& key, const Value& value);

        // Set value in table with write barrier support
        void setWithBarrier(const Value& key, const Value& value, LuaState* L);
        
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
        GCRef<Table> getMetatable() const { return metatable; }

        // Set metatable
        void setMetatable(GCRef<Table> mt) { metatable = mt; }
        
        // Clear weak references (for garbage collection)
        void clearWeakReferences();

        // Lua 5.1兼容的弱表支持
        bool hasWeakKeys() const;
        bool hasWeakValues() const;
        void setWeakKeys(bool weak);
        void setWeakValues(bool weak);

        // 检查表是否为弱表
        bool isWeakTable() const { return hasWeakKeys() || hasWeakValues(); }
        
    private:
        // Find key in entries
        int findEntry(const Value& key) const;
    };

    // Helper function to create GC-managed tables
    GCRef<Table> make_gc_table();
}
