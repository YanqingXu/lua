/**
 * @file table.cpp
 * @brief Lua表系统实现
 */

#include "core/table.hpp"
#include "common/lua_error.hpp"
#include "core/gc_string.hpp"
#include "core/function.hpp"
#include "gc/garbage_collector.hpp"
#include "vm/state/global_state.hpp"
#include <cmath>
#include <limits>

namespace Lua {

// =====================================================================
// ValueHash 实现
// =====================================================================

usize ValueHash::operator()(const Value& val) const noexcept {
    switch (val.getType()) {
    case ValueType::Nil:
        return 0;

    case ValueType::Boolean:
        return val.asBoolean() ? 1 : 0;

    case ValueType::Number: {
        f64 num = val.asNumber();
        // 使用std::hash<f64>计算浮点数的哈希值
        return std::hash<f64>{}(num);
    }

    case ValueType::LightUserdata: {
        void* ptr = val.asLightUserdata();
        // 使用指针值的哈希
        return std::hash<void*>{}(ptr);
    }

    case ValueType::String: {
        GCString* str = val.asString();
        // 使用字符串对象的预计算哈希值
        return str->getHash();
    }

    case ValueType::Table:
    case ValueType::Function:
    case ValueType::Userdata:
    case ValueType::Thread: {
        // GC对象使用指针值的哈希
        void* ptr = nullptr;
        if (val.getType() == ValueType::Table) {
            ptr = val.asTable();
        } else if (val.getType() == ValueType::Function) {
            ptr = val.asFunction();
        } else if (val.getType() == ValueType::Userdata) {
            ptr = val.asUserdata();
        } else if (val.getType() == ValueType::Thread) {
            ptr = val.asThread();
        }
        return std::hash<void*>{}(ptr);
    }

    default:
        return 0;
    }
}

// =====================================================================
// Table 实现
// =====================================================================

Table::Table() : Table(nullptr) {}

Table::Table(LuaAllocator* allocator)
    : GCObject(GCObjectType::Table), array_(allocator), hashNodes_(allocator), allocator_(allocator),
      metatable_(nullptr), flags_(0) // 初始化标志位为0（所有元方法都可能存在）
{}

Table::~Table() {
    if (GarbageCollector* owner = getOwnerCollector()) {
        owner->unregisterObject(this);
    }
    // 数组和哈希部分会自动释放
    // GC对象由GC系统管理，不需要手动释放
}

// =====================================================================
// 基本操作
// =====================================================================

Value Table::get(const Value& key) const {
    // 检查是否是数组索引
    i32 index;
    if (isPositiveIntegerKey(key, index) && static_cast<usize>(index) <= array_.size()) {
        Value value = getArray(index);
        if (!value.isNil()) {
            return value;
        }
    }

    // 从哈希部分查找
    const usize node = findHashNode(key, false);
    if (node != NoHashNode) {
        return hashNodes_[node].value;
    }

    // 键不存在，返回nil
    return Value(); // 默认构造函数创建nil
}

void Table::set(const Value& key, const Value& value) {
    // Lua语义：nil键不允许
    if (key.isNil()) {
        throw RuntimeError("table index is nil");
    }
    if (key.isNumber() && std::isnan(key.asNumber())) {
        throw RuntimeError("table index is NaN");
    }

    flags_ = 0;

    // 如果value是nil，表示删除该键
    if (value.isNil()) {
        remove(key);
        return;
    }

    // 检查是否是数组索引
    i32 index;
    if (isPositiveIntegerKey(key, index) && shouldStoreInArray(index)) {
        setArray(index, value);
        return;
    }
    setHash(key, value);
}

bool Table::has(const Value& key) const {
    // 检查是否是数组索引
    i32 index;
    if (isPositiveIntegerKey(key, index) && static_cast<usize>(index) <= array_.size()) {
        if (index >= 1 && static_cast<usize>(index) <= array_.size()) {
            if (!array_[index - 1].isNil()) {
                return true;
            }
        }
    }

    // 检查哈希部分
    return findHashNode(key, false) != NoHashNode;
}

void Table::remove(const Value& key) {
    flags_ = 0;

    // 检查是否是数组索引
    i32 index;
    if (isPositiveIntegerKey(key, index) && static_cast<usize>(index) <= array_.size() &&
        !array_[static_cast<usize>(index - 1)].isNil()) {
        if (index >= 1 && static_cast<usize>(index) <= array_.size()) {
            array_[index - 1] = Value(); // 默认构造函数创建nil
        }
    }

    // 从哈希部分删除
    removeHash(key);
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
}

void Table::clear() {
    array_.clear();
    hashNodes_ = LuaReallocVector<HashNode>(allocator_);
    hashLiveCount_ = 0;
    hashUsedCount_ = 0;
    metatable_ = nullptr;
    flags_ = 0;
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
}

// =====================================================================
// 数组操作
// =====================================================================

Value Table::getArray(i32 index) const {
    // Lua数组是1-based
    if (index < 1) {
        return Value(); // 默认构造函数创建nil
    }

    usize arrayIndex = static_cast<usize>(index - 1);
    if (arrayIndex < array_.size()) {
        return array_[arrayIndex];
    }

    // 索引超出范围，返回nil
    return Value(); // 默认构造函数创建nil
}

void Table::setArray(i32 index, const Value& value) {
    // Lua数组是1-based
    if (index < 1) {
        // TODO: 应该抛出错误，当前简单忽略
        return;
    }

    if (value.isNil() && static_cast<usize>(index) > array_.size()) {
        removeHash(Value(static_cast<f64>(index)));
        return;
    }
    const Value stableValue = value;
    setArrayRange(index, std::span<const Value>(&stableValue, 1));
}

void Table::setArrayRange(i32 firstIndex, std::span<const Value> values) {
    if (firstIndex < 1 || values.empty()) {
        return;
    }
    const usize first = static_cast<usize>(firstIndex - 1);
    if (values.size() > std::numeric_limits<usize>::max() - first) {
        throw std::bad_array_new_length();
    }

    if (GarbageCollector* gc = getOwnerCollector()) {
        // Complete all potentially allocating barriers before changing the
        // array. The subsequent realloc has a strong failure guarantee and
        // Value assignment is non-throwing.
        for (const Value& value : values) {
            gc->writeBarrier(this, value);
        }
    }

    const usize requiredSize = first + values.size();
    if (requiredSize > resourcePolicy().maxTableArraySlots) {
        throw ResourceLimitError("table array slot limit exceeded");
    }
    if (requiredSize > array_.size()) {
        array_.resize(requiredSize, Value());
    }
    flags_ = 0;
    for (usize offset = 0; offset < values.size(); ++offset) {
        array_[first + offset] = values[offset];
        removeHash(Value(static_cast<f64>(first + offset + 1)));
    }
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
}

void Table::setMetatable(Table* mt) {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, mt);
    }

    metatable_ = mt;
}

usize Table::length() const {
    auto hasIntegerKey = [this](usize index) -> bool {
        if (index == 0 || index > static_cast<usize>(std::numeric_limits<i32>::max())) {
            return false;
        }

        Value key(static_cast<f64>(index));
        i32 arrayIndex = 0;
        if (isPositiveIntegerKey(key, arrayIndex) && static_cast<usize>(arrayIndex) <= array_.size() &&
            !array_[static_cast<usize>(arrayIndex - 1)].isNil()) {
            return true;
        }

        return findHashNode(key, false) != NoHashNode;
    };

    usize arraySize = array_.size();
    if (arraySize > 0) {
        if (array_[arraySize - 1].isNil()) {
            usize low = 0;
            usize high = arraySize;
            while (high - low > 1) {
                usize mid = low + (high - low) / 2;
                if (array_[mid - 1].isNil()) {
                    high = mid;
                } else {
                    low = mid;
                }
            }
            return low;
        }

        if (!hasIntegerKey(arraySize + 1)) {
            return arraySize;
        }
    }

    usize low = arraySize;
    usize high = arraySize == 0 ? 1 : arraySize * 2;
    while (hasIntegerKey(high)) {
        low = high;
        if (high > static_cast<usize>(std::numeric_limits<i32>::max()) / 2) {
            return high;
        }
        high *= 2;
    }

    while (high - low > 1) {
        usize mid = low + (high - low) / 2;
        if (hasIntegerKey(mid)) {
            low = mid;
        } else {
            high = mid;
        }
    }

    return low;
}

// =====================================================================
// GCObject接口实现
// =====================================================================

void Table::mark(GarbageCollector& gc) {
    gc.markTable(this);
}

void Table::markContents(GarbageCollector& gc, bool weakKeys, bool weakValues) {
    // 标记数组部分中的GC对象
    if (!weakValues) {
        for (const Value& val : array_) {
            gc.markValue(val);
        }
    } else {
        for (const Value& val : array_) {
            if (val.isString()) {
                gc.markValue(val);
            }
        }
    }

    // 标记哈希部分中的GC对象
    for (const HashNode& node : hashNodes_) {
        if (node.state != HashNodeState::Live) {
            continue;
        }
        if (!weakKeys || node.key.isString()) {
            gc.markValue(node.key);
        }
        if (!weakValues || node.value.isString()) {
            gc.markValue(node.value);
        }
    }

    // 标记元表
    gc.markObject(metatable_);
}

void Table::removeWeakEntries(const GarbageCollector& gc, bool weakKeys, bool weakValues) {
    if (!weakKeys && !weakValues) {
        return;
    }

    if (weakValues) {
        for (Value& val : array_) {
            if (gc.isWeakValueDead(val)) {
                val = Value();
            }
        }
    }

    for (HashNode& node : hashNodes_) {
        if (node.state != HashNodeState::Live) {
            continue;
        }
        bool removeEntry = false;
        if (weakKeys && gc.isValueDead(node.key)) {
            removeEntry = true;
        }
        if (weakValues && gc.isWeakValueDead(node.value)) {
            removeEntry = true;
        }

        if (removeEntry) {
            node.value = Value();
            node.state = HashNodeState::Dead;
            --hashLiveCount_;
        }
    }

    if (GarbageCollector* owner = getOwnerCollector()) {
        owner->accountObjectSizeChange(this);
    }
}

usize Table::getSize() const {
    // Table对象本身的大小
    usize size = sizeof(Table);

    // 数组部分的容量
    size += array_.capacity() * sizeof(Value);

    // 开放寻址节点数组由 lua_Alloc 精确按 capacity 计账。
    size += hashNodes_.capacity() * sizeof(HashNode);

    return size;
}

// =====================================================================
// 迭代器支持
// =====================================================================

bool Table::next(const Value& key, Value& nextKey, Value& nextValue) const {
    // 如果key是nil，从头开始遍历
    if (key.isNil()) {
        // 先检查数组部分
        if (!array_.empty()) {
            // 返回第一个非nil的数组元素
            for (usize i = 0; i < array_.size(); i++) {
                if (!array_[i].isNil()) {
                    nextKey = Value(static_cast<f64>(i + 1)); // Lua索引从1开始
                    nextValue = array_[i];
                    return true;
                }
            }
        }

        // 数组部分为空或全是nil，检查哈希部分
        const usize firstNode = nextLiveHashNode(0);
        if (firstNode != NoHashNode) {
            nextKey = hashNodes_[firstNode].key;
            nextValue = hashNodes_[firstNode].value;
            return true;
        }

        // 表为空
        return false;
    }

    // 查找当前键的位置
    i32 arrayIndex;
    if (isPositiveIntegerKey(key, arrayIndex) && static_cast<usize>(arrayIndex) <= array_.size() &&
        (!array_[static_cast<usize>(arrayIndex - 1)].isNil() || findHashNode(key, true) == NoHashNode)) {
        // 当前键在数组部分
        // 继续遍历数组部分
        for (usize i = arrayIndex; i < array_.size(); i++) {
            if (!array_[i].isNil()) {
                nextKey = Value(static_cast<f64>(i + 1));
                nextValue = array_[i];
                return true;
            }
        }

        // 数组部分遍历完毕，转到哈希部分
        const usize firstNode = nextLiveHashNode(0);
        if (firstNode != NoHashNode) {
            nextKey = hashNodes_[firstNode].key;
            nextValue = hashNodes_[firstNode].value;
            return true;
        }

        return false;
    }

    // 当前键在哈希部分
    const usize currentNode = findHashNode(key, true);
    if (currentNode == NoHashNode) {
        throw RuntimeError("invalid key to 'next'");
    }

    const usize followingNode = nextLiveHashNode(currentNode + 1);
    if (followingNode != NoHashNode) {
        nextKey = hashNodes_[followingNode].key;
        nextValue = hashNodes_[followingNode].value;
        return true;
    }

    // 哈希部分遍历完毕
    return false;
}

// =====================================================================
// 内部辅助方法
// =====================================================================

bool Table::isPositiveIntegerKey(const Value& key, i32& outIndex) const {
    // 必须是数字类型
    if (!key.isNumber()) {
        return false;
    }

    f64 num = key.asNumber();

    // 必须是正整数
    if (num <= 0 || num != std::floor(num)) {
        return false;
    }

    if (num > static_cast<f64>(std::numeric_limits<i32>::max())) {
        return false;
    }

    // 转换为整数
    i32 index = static_cast<i32>(num);

    outIndex = index;
    return true;
}

bool Table::shouldStoreInArray(i32 index) const {
    if (index < 1) {
        return false;
    }
    const usize candidate = static_cast<usize>(index);
    if (candidate > resourcePolicy().maxTableArraySlots) {
        return false;
    }
    // Only contiguous growth is dense by construction. Sparse positive
    // integers remain in the node array instead of amplifying one value into
    // a large nil-filled allocation.
    return candidate <= array_.size() || candidate == array_.size() + 1;
}

const ResourcePolicy& Table::resourcePolicy() const noexcept {
    if (const GarbageCollector* gc = getOwnerCollector()) {
        if (const GlobalState* global = gc->getGlobalState()) {
            return global->getResourcePolicy();
        }
    }
    static const ResourcePolicy defaults;
    return defaults;
}

usize Table::findHashNode(const Value& key, bool includeDead) const noexcept {
    if (hashNodes_.empty()) {
        return NoHashNode;
    }

    const usize hash = ValueHash{}(key);
    const usize mask = hashNodes_.size() - 1;
    usize index = hash & mask;
    for (usize probes = 0; probes < hashNodes_.size(); ++probes) {
        const HashNode& node = hashNodes_[index];
        if (node.state == HashNodeState::Empty) {
            return NoHashNode;
        }
        if (node.hash == hash && node.key == key &&
            (node.state == HashNodeState::Live || includeDead)) {
            return index;
        }
        index = (index + 1) & mask;
    }
    return NoHashNode;
}

usize Table::nextLiveHashNode(usize first) const noexcept {
    for (usize index = first; index < hashNodes_.size(); ++index) {
        if (hashNodes_[index].state == HashNodeState::Live) {
            return index;
        }
    }
    return NoHashNode;
}

void Table::setHash(const Value& key, const Value& value) {
    usize nodeIndex = findHashNode(key, true);
    if (nodeIndex != NoHashNode && hashNodes_[nodeIndex].state == HashNodeState::Live) {
        if (GarbageCollector* gc = getOwnerCollector()) {
            gc->writeBarrier(this, value);
        }
        hashNodes_[nodeIndex].value = value;
        if (GarbageCollector* gc = getOwnerCollector()) {
            gc->accountObjectSizeChange(this);
        }
        return;
    }

    if (hashLiveCount_ >= resourcePolicy().maxTableHashEntries) {
        throw ResourceLimitError("table hash entry limit exceeded");
    }
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, key);
        gc->writeBarrier(this, value);
    }

    if (nodeIndex != NoHashNode) {
        HashNode& node = hashNodes_[nodeIndex];
        node.value = value;
        node.state = HashNodeState::Live;
        ++hashLiveCount_;
    } else {
        ensureHashInsertCapacity();
        const usize hash = ValueHash{}(key);
        const usize mask = hashNodes_.size() - 1;
        usize index = hash & mask;
        usize firstDead = NoHashNode;
        for (;;) {
            HashNode& node = hashNodes_[index];
            if (node.state == HashNodeState::Dead && firstDead == NoHashNode) {
                firstDead = index;
            } else if (node.state == HashNodeState::Empty) {
                const usize destination = firstDead == NoHashNode ? index : firstDead;
                HashNode& target = hashNodes_[destination];
                target.key = key;
                target.value = value;
                target.hash = hash;
                target.state = HashNodeState::Live;
                ++hashLiveCount_;
                if (firstDead == NoHashNode) {
                    ++hashUsedCount_;
                }
                break;
            }
            index = (index + 1) & mask;
        }
    }

    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
}

void Table::removeHash(const Value& key) noexcept {
    const usize index = findHashNode(key, false);
    if (index == NoHashNode) {
        return;
    }
    HashNode& node = hashNodes_[index];
    node.value = Value();
    node.state = HashNodeState::Dead;
    --hashLiveCount_;
}

void Table::ensureHashInsertCapacity() {
    if (hashNodes_.empty()) {
        rehash(8);
        return;
    }
    if (hashUsedCount_ + 1 <= hashNodes_.size() - hashNodes_.size() / 4) {
        return;
    }
    if (hashLiveCount_ * 2 < hashUsedCount_) {
        rehash(hashNodes_.size());
        return;
    }
    if (hashNodes_.size() > std::numeric_limits<usize>::max() / 2) {
        throw std::bad_array_new_length();
    }
    rehash(hashNodes_.size() * 2);
}

void Table::rehash(usize requestedCapacity) {
    usize capacity = 8;
    while (capacity < requestedCapacity) {
        if (capacity > std::numeric_limits<usize>::max() / 2) {
            throw std::bad_array_new_length();
        }
        capacity *= 2;
    }

    LuaReallocVector<HashNode> replacement(allocator_);
    replacement.resize(capacity, HashNode{});
    const usize mask = capacity - 1;
    for (const HashNode& old : hashNodes_) {
        if (old.state != HashNodeState::Live) {
            continue;
        }
        usize index = old.hash & mask;
        while (replacement[index].state == HashNodeState::Live) {
            index = (index + 1) & mask;
        }
        replacement[index] = old;
    }
    hashNodes_ = std::move(replacement);
    hashUsedCount_ = hashLiveCount_;
}

} // namespace Lua
