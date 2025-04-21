#pragma once

#include "../types.hpp"
#include "value.hpp"
#include <vector>

namespace Lua {
    // 简化的Table实现
    class Table {
    private:
        // 使用简单的共享元素元组存储键值对
        struct Entry {
            Value key;
            Value value;
            
            // 添加全套的构造、赋值和移动操作
            Entry() = default;
            Entry(const Value& k, const Value& v) : key(k), value(v) {}
            Entry(const Entry& other) = default;
            Entry(Entry&& other) noexcept = default;
            Entry& operator=(const Entry& other) = default;
            Entry& operator=(Entry&& other) noexcept = default;
            ~Entry() = default;
        };
        
        // 数组部分
        Vec<Value> array;
        
        // 哈希表部分用简单的向量实现
        Vec<Entry> entries;
        
    public:
        Table() = default;
        
        // 获取表中的值
        Value get(const Value& key);
        
        // 设置表中的值
        void set(const Value& key, const Value& value);
        
        // 获取表的长度
        size_t length() const { return array.size(); }
        
    private:
        // 在entries中查找键
        int findEntry(const Value& key) const;
    };
}
