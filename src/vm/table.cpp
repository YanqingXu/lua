#include "table.hpp"
#include <cmath>        // For std::floor

namespace Lua {
    // Find the index of a key in entries, return -1 if not found
    int Table::findEntry(const Value& key) const {
        for (size_t i = 0; i < entries.size(); i++) {
            if (entries[i].key == key) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    Value Table::get(const Value& key) {
        // If it's an integer key and within array range
        if (key.isNumber()) {
            LuaNumber n = key.asNumber();
            if (n == std::floor(n) && n >= 1 && n <= array.size()) {
                size_t index = static_cast<size_t>(n - 1);  // Lua index starts from 1
                return array[index];
            }
        }
        
        // Search in entries
        int index = findEntry(key);
        if (index >= 0) {
            return entries[index].value;
        }
        
        // Return nil if not found
        return Value(nullptr);
    }
    
    void Table::set(const Value& key, const Value& value) {
        // Cannot use nil as key
        if (key.isNil()) {
            return;
        }
        
        // Handle integer keys, try to store in array part
        if (key.isNumber()) {
            LuaNumber n = key.asNumber();
            if (n == std::floor(n) && n >= 1) {  // Is integer and >= 1
                size_t index = static_cast<size_t>(n - 1);  // Lua index starts from 1
                
                // If need to expand array
                if (index >= array.size()) {
                    if (value.isNil()) {
                        return;  // No need to expand array for nil value
                    }
                    array.resize(index + 1, Value(nullptr));
                }
                
                array[index] = value;
                return;
            }
        }
        
        // Find existing key
        int index = findEntry(key);
        
        if (value.isNil()) {
            // Delete element (if exists)
            if (index >= 0) {
                // Swap the element to be deleted with the last element, then remove the last element
                // This avoids holes in the vector
                if (index < static_cast<int>(entries.size()) - 1) {
                    entries[index] = entries.back();
                }
                entries.pop_back();
            }
        } else {
            // Update existing element
            if (index >= 0) {
                entries[index].value = value;
            } else {
                // Add new element
                entries.emplace_back(key, value);
            }
        }
    }
}
