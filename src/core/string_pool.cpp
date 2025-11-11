/**
 * @file string_pool.cpp
 * @brief StringPool类的实现
 */

#include "core/string_pool.hpp"

namespace Lua {

/**
 * @brief 驻留字符串 - 获取或创建字符串对象
 */
GCString* StringPool::intern(StrView str) {
    // 将StrView转换为Str用于查找
    Str key(str);

    // 在池中查找是否已存在
    auto it = pool_.find(key);
    if (it != pool_.end()) {
        // 已存在，返回已有的字符串对象
        return it->second;
    }

    // 不存在，创建新的字符串对象
    GCString* newString = new GCString(str);

    // 加入池中
    pool_[key] = newString;

    return newString;
}

/**
 * @brief 查找字符串 - 不创建新对象
 */
GCString* StringPool::find(StrView str) const {
    Str key(str);

    auto it = pool_.find(key);
    if (it != pool_.end()) {
        return it->second;
    }

    return nullptr;
}

/**
 * @brief 从池中移除字符串
 */
void StringPool::remove(GCString* str) {
    if (str == nullptr) {
        return;
    }
    
    // 使用字符串内容作为key查找并移除
    pool_.erase(str->getData());
}

/**
 * @brief 清空字符串池
 */
void StringPool::clear() {
    pool_.clear();
}

} // namespace Lua

