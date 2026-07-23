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
    if (resourcePolicy_ != nullptr && str.size() > resourcePolicy_->maxStringBytes) {
        throw ResourceLimitError("resource limit exceeded: string bytes");
    }

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
        /**
         * @brief 使用 GCString 拥有的内容作为键，使 find() 与 remove() 共享同一规范值。
         *
         * 此时对象已注册到垃圾回收器，因此插入失败必须回滚，不能在对象链表中留下未驻留的
         * GCString。
         */
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

    /**
     * @brief 仅当条目仍指向正在销毁的确切对象时才将其移除。
     *
     * 垃圾回收器可能短暂拥有内容相同的另一 GCString，例如回滚失败的插入期间。
     */
    auto it = pool_.find(str->view());
    if (it != pool_.end() && it->second == str) {
        pool_.erase(it);
    }
}

/**
 * @brief 清空字符串驻留池
 */
void StringPool::clear() {
    pool_.clear();
}

/**
 * @brief 调整字符串驻留池大小（预分配）
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
