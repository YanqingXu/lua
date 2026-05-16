/**
 * @file string_pool.cpp
 * @brief StringPool类的实现
 */

#include "core/string_pool.hpp"
#include "gc/garbage_collector.hpp"

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
    GarbageCollector::getInstance().registerObject(newString);

    // 加入池中
    // 使用GCString内部的data_作为key，确保与find()和remove()一致
    pool_[newString->getData()] = newString;

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

/**
 * @brief 调整字符串池大小（预分配）
 *
 * 预分配哈希表空间，减少后续插入时的重哈希开销。
 *
 * 实现说明：
 * C++的unordered_map::reserve()会预分配足够的桶（buckets）来容纳
 * 至少newSize个元素而不触发重哈希。这与Lua 5.1.5的luaS_resize()
 * 功能等价，但实现更简单。
 *
 * @param newSize 新的哈希表大小
 *
 * @note 对应C实现的 luaS_resize()
 * @see lua_c_analysis/src/lstring.c 第93-130行
 */
void StringPool::resize(usize newSize) {
    pool_.reserve(newSize);
}

} // namespace Lua

