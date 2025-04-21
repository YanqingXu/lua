#include "table.hpp"
#include <cmath>        // 用于std::floor

namespace Lua {
    // 在entries中查找键的索引，如果找不到返回-1
    int Table::findEntry(const Value& key) const {
        for (size_t i = 0; i < entries.size(); i++) {
            if (entries[i].key == key) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    Value Table::get(const Value& key) {
        // 如果是整数键且在数组范围内
        if (key.isNumber()) {
            LuaNumber n = key.asNumber();
            if (n == std::floor(n) && n >= 1 && n <= array.size()) {
                size_t index = static_cast<size_t>(n - 1);  // Lua索引从1开始
                return array[index];
            }
        }
        
        // 在entries中查找
        int index = findEntry(key);
        if (index >= 0) {
            return entries[index].value;
        }
        
        // 找不到返回nil
        return Value(nullptr);
    }
    
    void Table::set(const Value& key, const Value& value) {
        // 不能使用nil作为键
        if (key.isNil()) {
            return;
        }
        
        // 处理整数键，尝试存储在数组部分
        if (key.isNumber()) {
            LuaNumber n = key.asNumber();
            if (n == std::floor(n) && n >= 1) {  // 是整数且大于等于1
                size_t index = static_cast<size_t>(n - 1);  // Lua索引从1开始
                
                // 如果需要扩展数组
                if (index >= array.size()) {
                    if (value.isNil()) {
                        return;  // 不需要为nil值扩展数组
                    }
                    array.resize(index + 1, Value(nullptr));
                }
                
                array[index] = value;
                return;
            }
        }
        
        // 查找现有键
        int index = findEntry(key);
        
        if (value.isNil()) {
            // 删除元素（如果存在）
            if (index >= 0) {
                // 将要删除的元素与最后一个元素交换，然后移除最后一个元素
                // 这样避免了向量中的空洞
                if (index < static_cast<int>(entries.size()) - 1) {
                    entries[index] = entries.back();
                }
                entries.pop_back();
            }
        } else {
            // 更新现有元素
            if (index >= 0) {
                entries[index].value = value;
            } else {
                // 添加新元素
                entries.emplace_back(key, value);
            }
        }
    }
}
