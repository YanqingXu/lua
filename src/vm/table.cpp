#include "table.hpp"
#include "value.hpp"
#include "../gc/memory/allocator.hpp"
#include "../gc/core/gc_ref.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "table_impl.hpp"
#include <functional>   // For std::function

namespace Lua {
    
    // Table destructor to clean up dynamically allocated entries
    Table::~Table() {
        for (void* entryPtr : entries) {
            delete static_cast<Entry*>(entryPtr);
        }
    }
    
    // Find the index of a key in entries, return -1 if not found
    int Table::findEntry(const Value& key) const {
        for (size_t i = 0; i < entries.size(); i++) {
            Entry* entry = static_cast<Entry*>(entries[i]);
            if (entry->key == key) {
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
            Entry* entry = static_cast<Entry*>(entries[index]);
            return entry->value;
        }

        // Return nil if not found
        return Value(nullptr);
    }

    Value Table::get(const Value& key) const {
        // If it's an integer key and within array range
        if (key.isNumber()) {
            LuaNumber n = key.asNumber();
            if (n == std::floor(n) && n >= 1 && n <= array.size()) {
                size_t index = static_cast<size_t>(n - 1);  // Lua index starts from 1
                return array[index];
            }
        }

        // Search in entries (const version)
        for (size_t i = 0; i < entries.size(); ++i) {
            if (entries[i]) {
                Entry* entry = static_cast<Entry*>(entries[i]);
                if (entry->key == key) {
                    return entry->value;
                }
            }
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
                // Delete the entry and swap with last element
                delete static_cast<Entry*>(entries[index]);
                if (index < static_cast<int>(entries.size()) - 1) {
                    entries[index] = entries.back();
                }
                entries.pop_back();
            }
        } else {
            // Update existing element
            if (index >= 0) {
                Entry* entry = static_cast<Entry*>(entries[index]);
                entry->value = value;
            } else {
                // Add new element
                Entry* newEntry = new Entry(key, value);
                entries.push_back(newEntry);
            }
        }
    }
    
    // GCObject virtual function implementations
    void Table::markReferences(GarbageCollector* gc) {
        // Mark array part
        for (const auto& value : array) {
            if (value.isGCObject()) {
                gc->markObject(value.asGCObject());
            }
        }
        
        // Mark hash part
        for (const auto& entryPtr : entries) {
            Entry* entry = static_cast<Entry*>(entryPtr);
            if (entry->key.isGCObject()) {
                gc->markObject(entry->key.asGCObject());
            }
            if (entry->value.isGCObject()) {
                gc->markObject(entry->value.asGCObject());
            }
        }
        
        // Mark metatable if present
        if (metatable) {
            gc->markObject(metatable.get());
        }
    }
    
    void Table::clearWeakReferences() {
        // Clear weak references in array part
        // For now, we don't implement weak references, so this is a no-op
        // In a full implementation, this would remove entries where the key or value
        // is a weak reference to a collected object
        
        // Clear weak references in hash part
        // This would iterate through hashPart and remove entries with collected weak references
        
        // Note: This is a placeholder implementation
        // Full weak reference support would require additional metadata
    }
    
    usize Table::getSize() const {
        return sizeof(Table);
    }
    
    usize Table::getAdditionalSize() const {
        // Calculate additional memory used by vectors
        usize arraySize = array.capacity() * sizeof(Value);
        usize entriesSize = entries.capacity() * sizeof(void*) + entries.size() * sizeof(Entry);
        return arraySize + entriesSize;
    }
    
    size_t Table::length() const {
        // Lua 5.1 table length: find the largest consecutive integer index starting from 1
        size_t length = 0;

        // Check consecutive integer keys starting from 1
        for (size_t i = 1; ; ++i) {
            Value key(static_cast<double>(i));
            Value value = get(key);
            if (value.isNil()) {
                break; // Found the first nil value, length is i-1
            }
            length = i;
        }

        return length;
    }
    
    usize Table::getArraySize() const {
        return array.size();
    }
    
    const Value& Table::getArrayElement(usize index) const {
        return array[index];
    }
    
    
    // Implementation of make_gc_table to avoid circular dependencies
    GCRef<Table> make_gc_table() {
        extern GCAllocator* g_gcAllocator;
        if (g_gcAllocator) {
            Table* obj = g_gcAllocator->allocateObject<Table>(GCObjectType::Table);
            return GCRef<Table>(obj);
        }
        
        // Fallback to direct allocation
        Table* obj = new Table();
        return GCRef<Table>(obj);
    }
}
