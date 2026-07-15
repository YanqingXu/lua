/**
 * @file string_pool.cpp
 * @brief StringPool类的实现
 */

#include "core/string_pool.hpp"
#include "gc/garbage_collector.hpp"

namespace Lua {

StringPool::StringPool(LuaAllocator* allocator)
    : pool_(0, std::hash<StrView>{}, std::equal_to<StrView>{}, PoolAllocator(allocator)) {}

void StringPool::setGarbageCollector(GarbageCollector* collector) {
    collector_ = collector;
    if (collector_ == nullptr) {
        return;
    }

    collector_->setStringPool(this);

    for (auto& entry : pool_) {
        collector_->registerObject(entry.second);
    }
}

/**
 * @brief 驻留字符串 - 获取或创建字符串对象
 */
GCString* StringPool::intern(StrView str) {
    // 在池中查找是否已存在
    auto it = pool_.find(str);
    if (it != pool_.end()) {
        // 已存在，返回已有的字符串对象
        return it->second;
    }

    // 不存在，创建新的字符串对象
    GarbageCollector& gc = collector_ != nullptr ? *collector_ : GarbageCollector::legacyInstance();
    gc.setStringPool(this);
    GCString* newString = gc.create<GCString>(str);

    try {
        // Use the GCString-owned contents as the key so find() and remove()
        // share the same canonical value. The object is already registered
        // with the collector at this point, so insertion failure must roll it
        // back instead of leaving an uninterned GCString in the object list.
        auto [entry, inserted] = pool_.emplace(newString->view(), newString);
        if (!inserted) {
            gc.destroyManagedObject(newString);
            return entry->second;
        }
    } catch (...) {
        gc.destroyManagedObject(newString);
        throw;
    }

    return newString;
}

/**
 * @brief 查找字符串 - 不创建新对象
 */
GCString* StringPool::find(StrView str) const {
    auto it = pool_.find(str);
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

    // A collector can transiently own another GCString with equal contents
    // (for example while rolling back a failed insertion). Only erase the
    // entry when it still names the exact object being destroyed.
    auto it = pool_.find(str->view());
    if (it != pool_.end() && it->second == str) {
        pool_.erase(it);
    }
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
 */
void StringPool::resize(usize newSize) {
    pool_.reserve(newSize);
}

} // namespace Lua
