#include "table.hpp"
#include "value.hpp"
#include "../gc/memory/allocator.hpp"
#include "../gc/core/gc_ref.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../api/lua51_gc_api.hpp"
#include "table_impl.hpp"
#include "metamethod_manager.hpp"
#include "lua_state.hpp"
#include <functional>   // For std::function
#include <iostream>

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

    Value Table::getWithMetamethod(const Value& key, LuaState* state) {
        // First try direct lookup
        Value result = get(key);
        if (!result.isNil()) {
            return result;
        }

        // If not found and we have a metatable, try __index metamethod
        if (metatable && state) {
            Value indexMethod = MetaMethodManager::getMetaMethod(metatable, MetaMethod::Index);
            if (!indexMethod.isNil()) {
                if (indexMethod.isFunction()) {
                    // Call __index function with (table, key)
                    Vec<Value> args = {Value(GCRef<Table>(this)), key};
                    try {
                        return state->callFunction(indexMethod, args);
                    } catch (const std::exception& /*e*/) {
                        // If metamethod call fails, return nil
                        return Value();
                    }
                } else if (indexMethod.isTable()) {
                    // Recursively look up in __index table
                    auto indexTable = indexMethod.asTable();
                    if (indexTable) {
                        return indexTable->getWithMetamethod(key, state);
                    }
                }
            }
        }

        // Return nil if no metamethod or metamethod didn't provide a value
        return Value();
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
        // 实现Lua 5.1兼容的弱引用清理
        if (!isWeakTable()) {
            return; // 不是弱表，无需清理
        }

        bool weakKeys = hasWeakKeys();
        bool weakValues = hasWeakValues();

        // 清理数组部分的弱引用
        if (weakValues) {
            for (auto& value : array) {
                if (value.isGCObject()) {
                    // 简化实现：暂时跳过实际的死亡检查
                    // 在完整实现中，这里会检查对象是否已死亡
                    // GCObject* obj = value.asGCObject();
                    // if (obj && GCUtils::isdead(nullptr, obj)) {
                    //     value = Value(); // 设置为nil
                    // }
                }
            }
        }

        // 清理哈希部分的弱引用
        // 注意：这里需要实际的哈希表实现来支持
        // 当前的简化实现暂时跳过哈希部分的清理

        // TODO: 实现完整的哈希表弱引用清理
        // 需要遍历所有哈希条目并检查键值的存活状态
    }

    // Lua 5.1兼容的弱表支持实现
    bool Table::hasWeakKeys() const {
        return GCUtils::hasweakkeys(this);
    }

    bool Table::hasWeakValues() const {
        return GCUtils::hasweakvalues(this);
    }

    void Table::setWeakKeys(bool weak) {
        if (weak) {
            GCUtils::setweakkeys(const_cast<GCObject*>(static_cast<const GCObject*>(this)));
        } else {
            // 需要实现取消弱键的功能
            // 当前GCUtils中没有unsetweakkeys函数
        }
    }

    void Table::setWeakValues(bool weak) {
        if (weak) {
            GCUtils::setweakvalues(const_cast<GCObject*>(static_cast<const GCObject*>(this)));
        } else {
            // 需要实现取消弱值的功能
            // 当前GCUtils中没有unsetweakvalues函数
        }
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

    // === Write Barrier Support for Table Operations ===

    void Table::setWithBarrier(const Value& key, const Value& value, LuaState* L) {
        // 在设置值之前应用写屏障
        if (L && value.isGCObject()) {
            // 表对象引用新的GC对象时需要写屏障
            // 使用luaC_objbarriert宏，它专门用于表对象
            GCObject* valueObj = nullptr;

            // 获取GCObject指针
            valueObj = value.asGCObject();

            if (valueObj) {
                luaC_barriert(L, this, valueObj);
            }
        }

        // 执行实际的set操作
        set(key, value);
    }
}
