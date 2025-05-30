#pragma once

#include "../types.hpp"
#include "value.hpp"
#include <vector>

namespace Lua {
    // Simplified Table implementation
    class Table {
    private:
        // Use simple shared element tuples to store key-value pairs
        struct Entry {
            Value key;
            Value value;
            
            // Add complete set of construction, assignment and move operations
            Entry() = default;
            Entry(const Value& k, const Value& v) : key(k), value(v) {}
            Entry(const Entry& other) = default;
            Entry(Entry&& other) noexcept = default;
            Entry& operator=(const Entry& other) = default;
            Entry& operator=(Entry&& other) noexcept = default;
            ~Entry() = default;
        };
        
        // Array part
        Vec<Value> array;
        
        // Hash table part implemented with simple vector
        Vec<Entry> entries;
        
    public:
        Table() = default;
        
        // Get value from table
        Value get(const Value& key);
        
        // Set value in table
        void set(const Value& key, const Value& value);
        
        // Get table length
        size_t length() const { return array.size(); }
        
    private:
        // Find key in entries
        int findEntry(const Value& key) const;
    };
}
