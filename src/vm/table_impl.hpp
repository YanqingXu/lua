#ifndef LUA_TABLE_IMPL_HPP
#define LUA_TABLE_IMPL_HPP

#include "table.hpp"
#include "value.hpp"

namespace Lua {
    // Entry structure definition (needed for template implementation)
    struct Table::Entry {
        Value key;
        Value value;
        
        Entry() = default;
        Entry(const Value& k, const Value& v) : key(k), value(v) {}
    };
    
    // Template implementation for forEachHashEntry
    template<typename Func>
    void Table::forEachHashEntry(Func&& func) const {
        for (const auto& entryPtr : entries) {
            Entry* entry = static_cast<Entry*>(entryPtr);
            if (!entry->key.isNil()) {
                func(entry->key, entry->value);
            }
        }
    }
}

#endif // LUA_TABLE_IMPL_HPP