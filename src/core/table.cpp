/**
 * @file table.cpp
 * @brief Lua表系统实现
 */

#include "core/table.hpp"
#include "common/lua_error.hpp"
#include "core/gc_string.hpp"
#include "core/function.hpp"
#include "gc/garbage_collector.hpp"
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

Table::Table()
    : GCObject(GCObjectType::Table)
    , metatable_(nullptr)
    , flags_(0)  // 初始化标志位为0（所有元方法都可能存在）
{
}

Table::~Table() {
    // 数组和哈希部分会自动释放
    // GC对象由GC系统管理，不需要手动释放
}

// =====================================================================
// 基本操作
// =====================================================================

Value Table::get(const Value& key) const {
    // 检查是否是数组索引
    i32 index;
    if (isArrayIndex(key, index)) {
        return getArray(index);
    }

    // 从哈希部分查找
    auto it = hash_.find(key);
    if (it != hash_.end()) {
        return it->second;
    }

    // 键不存在，返回nil
    return Value();  // 默认构造函数创建nil
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
    if (isArrayIndex(key, index)) {
        setArray(index, value);
        return;
    }

    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, key);
        gc->writeBarrier(this, value);
    }
    
    // 存储到哈希部分
    hash_[key] = value;
}

bool Table::has(const Value& key) const {
    // 检查是否是数组索引
    i32 index;
    if (isArrayIndex(key, index)) {
        if (index >= 1 && static_cast<usize>(index) <= array_.size()) {
            return !array_[index - 1].isNil();
        }
        return false;
    }

    // 检查哈希部分
    auto it = hash_.find(key);
    return it != hash_.end() && !it->second.isNil();
}

void Table::remove(const Value& key) {
    flags_ = 0;

    // 检查是否是数组索引
    i32 index;
    if (isArrayIndex(key, index)) {
        if (index >= 1 && static_cast<usize>(index) <= array_.size()) {
            array_[index - 1] = Value();  // 默认构造函数创建nil
        }
        return;
    }

    // 从哈希部分删除
    hash_.erase(key);
}

void Table::clear() {
    array_.clear();
    hash_.clear();
    metatable_ = nullptr;
    flags_ = 0;
}

// =====================================================================
// 数组操作
// =====================================================================

Value Table::getArray(i32 index) const {
    // Lua数组是1-based
    if (index < 1) {
        return Value();  // 默认构造函数创建nil
    }

    usize arrayIndex = static_cast<usize>(index - 1);
    if (arrayIndex < array_.size()) {
        return array_[arrayIndex];
    }

    // 索引超出范围，返回nil
    return Value();  // 默认构造函数创建nil
}

void Table::setArray(i32 index, const Value& value) {
    // Lua数组是1-based
    if (index < 1) {
        // TODO: 应该抛出错误，当前简单忽略
        return;
    }

    flags_ = 0;

    usize arrayIndex = static_cast<usize>(index - 1);

    // 如果索引超出当前大小，扩展数组
    if (arrayIndex >= array_.size()) {
        // 扩展数组，中间的空位填充nil
        array_.resize(arrayIndex + 1, Value());  // 默认构造函数创建nil
    }

    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, value);
    }

    array_[arrayIndex] = value;
}

void Table::setMetatable(Table* mt) noexcept {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, mt);
    }

    metatable_ = mt;
}

usize Table::length() const {
    auto hasIntegerKey = [this](usize index) -> bool {
        if (index == 0 ||
            index > static_cast<usize>(std::numeric_limits<i32>::max())) {
            return false;
        }

        Value key(static_cast<f64>(index));
        i32 arrayIndex = 0;
        if (isArrayIndex(key, arrayIndex)) {
            return static_cast<usize>(arrayIndex) <= array_.size() &&
                   !array_[static_cast<usize>(arrayIndex - 1)].isNil();
        }

        auto it = hash_.find(key);
        return it != hash_.end() && !it->second.isNil();
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
    for (const auto& [key, val] : hash_) {
        if (!weakKeys || key.isString()) {
            gc.markValue(key);
        }
        if (!weakValues || val.isString()) {
            gc.markValue(val);
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

    for (auto it = hash_.begin(); it != hash_.end(); ) {
        bool removeEntry = false;
        if (weakKeys && gc.isValueDead(it->first)) {
            removeEntry = true;
        }
        if (weakValues && gc.isWeakValueDead(it->second)) {
            removeEntry = true;
        }

        if (removeEntry) {
            it = hash_.erase(it);
        } else {
            ++it;
        }
    }
}

usize Table::getSize() const {
    // Table对象本身的大小
    usize size = sizeof(Table);
    
    // 数组部分的容量
    size += array_.capacity() * sizeof(Value);
    
    // 哈希部分的容量（估算）
    // unordered_map的内存布局比较复杂，这里简化估算
    size += hash_.size() * (sizeof(Value) * 2 + sizeof(void*));
    
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
                    nextKey = Value(static_cast<f64>(i + 1));  // Lua索引从1开始
                    nextValue = array_[i];
                    return true;
                }
            }
        }

        // 数组部分为空或全是nil，检查哈希部分
        if (!hash_.empty()) {
            auto it = hash_.begin();
            nextKey = it->first;
            nextValue = it->second;
            return true;
        }

        // 表为空
        return false;
    }

    // 查找当前键的位置
    i32 arrayIndex;
    if (isArrayIndex(key, arrayIndex)) {
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
        if (!hash_.empty()) {
            auto it = hash_.begin();
            nextKey = it->first;
            nextValue = it->second;
            return true;
        }

        return false;
    }

    // 当前键在哈希部分
    auto it = hash_.find(key);
    if (it == hash_.end()) {
        // Lua 5.1 permits deleting the current key during traversal. In that
        // case the iterator key no longer exists, so continue from any
        // remaining hash entry instead of terminating the traversal.
        if (!hash_.empty()) {
            auto restart = hash_.begin();
            nextKey = restart->first;
            nextValue = restart->second;
            return true;
        }
        return false;
    }

    // 移动到下一个元素
    ++it;
    if (it != hash_.end()) {
        nextKey = it->first;
        nextValue = it->second;
        return true;
    }

    // 哈希部分遍历完毕
    return false;
}

// =====================================================================
// 内部辅助方法
// =====================================================================

bool Table::isArrayIndex(const Value& key, i32& outIndex) const {
    // 必须是数字类型
    if (!key.isNumber()) {
        return false;
    }
    
    f64 num = key.asNumber();
    
    // 必须是正整数
    if (num <= 0 || num != std::floor(num)) {
        return false;
    }
    
    // 检查范围（避免过大的索引）
    // 这里设置一个合理的上限，比如1000000
    if (num < 1 || num > 1000000) {
        return false;
    }

    // 转换为整数
    i32 index = static_cast<i32>(num);
    
    outIndex = index;
    return true;
}

} // namespace Lua

