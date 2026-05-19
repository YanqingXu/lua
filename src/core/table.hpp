/**
 * @file table.hpp
 * @brief Lua表系统：混合数组/哈希表实现
 * 
 * 详细说明：
 * 这个文件实现了Lua最重要的数据结构——表（Table）。Lua的表是一种独特的
 * 数据结构，同时具备数组和哈希表的特性。设计采用混合存储策略：
 * - 数组部分：用于存储连续的正整数键（1, 2, 3, ...），使用Vec实现O(1)访问
 * - 哈希部分：用于存储其他类型的键或非连续的整数键，使用HashMap实现
 * 
 * 这种设计使得Lua表既能高效处理数组操作，又能灵活支持关联数组的需求。
 * 
 * 技术特点：
 * - 混合存储：数组部分和哈希部分的智能分配
 * - 动态调整：根据使用模式自动调整内存布局（当前版本简化实现）
 * - 元表支持：完整的元编程能力
 * - GC集成：继承自GCObject，支持垃圾回收
 * 
 * 参考实现：
 * - lua_c_analysis/src/ltable.h - Lua 5.1.5 C实现
 * - lua/docs/architecture/overview.md - 架构设计文档
 * 
 * @author YanqingXu
 * @date 2025-11-12
 * @version 1.0
 */

#pragma once

#include "common/types.hpp"
#include "core/gc_object.hpp"
#include "core/value.hpp"

namespace Lua {

// 前向声明
class Table;

/**
 * @brief Value类型的哈希函数对象
 * 
 * 为Value类型提供哈希函数，使其可以作为HashMap的键。
 * 不同类型的Value使用不同的哈希策略：
 * - Nil: 固定哈希值0
 * - Boolean: true为1，false为0
 * - Number: 使用std::hash<f64>
 * - LightUserdata: 使用指针值的哈希
 * - String/Table/Function/Userdata/Thread: 使用GC对象指针的哈希
 */
struct ValueHash {
    usize operator()(const Value& val) const noexcept;
};

/**
 * @brief Value类型的相等比较函数对象
 * 
 * 为Value类型提供相等比较，使其可以作为HashMap的键。
 * 直接使用Value类的operator==实现。
 */
struct ValueEqual {
    bool operator()(const Value& lhs, const Value& rhs) const noexcept {
        return lhs == rhs;
    }
};

/**
 * @brief Table类 - Lua表对象
 * 
 * 实现Lua的表数据结构，继承自GCObject以支持垃圾回收。
 * 
 * 内部结构：
 * - array_: 数组部分，存储连续的正整数键（索引从1开始）
 * - hash_: 哈希部分，存储其他类型的键或非连续的整数键
 * - metatable_: 元表指针，用于元编程
 * 
 * 使用示例：
 * @code
 * Table* t = new Table();
 * 
 * // 数组操作（1-based索引）
 * t->setArray(1, Value::makeNumber(42.0));
 * Value v1 = t->getArray(1);  // 42.0
 * 
 * // 哈希操作
 * t->set(Value::makeString(str), Value::makeBoolean(true));
 * Value v2 = t->get(Value::makeString(str));  // true
 * 
 * // 元表操作
 * Table* mt = new Table();
 * t->setMetatable(mt);
 * @endcode
 */
class Table : public GCObject {
public:
    /**
     * @brief 默认构造函数
     * 
     * 创建一个空表，数组部分和哈希部分都为空。
     */
    Table();
    
    /**
     * @brief 析构函数
     * 
     * 释放表占用的内存。注意：表中引用的GC对象由GC系统管理，
     * 这里不需要手动释放。
     */
    ~Table() override;
    
    // =====================================================================
    // 基本操作
    // =====================================================================
    
    /**
     * @brief 获取键对应的值
     * 
     * 查找策略：
     * 1. 如果key是正整数且在数组范围内，从数组部分获取
     * 2. 否则从哈希部分获取
     * 3. 如果键不存在，返回nil
     * 
     * @param key 要查找的键
     * @return 键对应的值，如果不存在返回nil
     */
    Value get(const Value& key) const;
    
    /**
     * @brief 设置键值对
     * 
     * 设置策略：
     * 1. 如果value是nil，表示删除该键
     * 2. 如果key是正整数且较小，存储到数组部分
     * 3. 否则存储到哈希部分
     * 
     * @param key 要设置的键（不能是nil）
     * @param value 要设置的值（nil表示删除）
     * 
     * @note Lua语义：nil键不允许，nil值表示删除
     * @warning 如果key是nil，行为未定义（应该抛出错误）
     */
    void set(const Value& key, const Value& value);
    
    /**
     * @brief 检查键是否存在
     * 
     * @param key 要检查的键
     * @return true 如果键存在且值不为nil
     */
    bool has(const Value& key) const;
    
    /**
     * @brief 删除键值对
     * 
     * 等价于 set(key, Value::makeNil())
     * 
     * @param key 要删除的键
     */
    void remove(const Value& key);
    
    // =====================================================================
    // 数组操作
    // =====================================================================
    
    /**
     * @brief 获取数组元素
     * 
     * Lua数组使用1-based索引，即第一个元素的索引是1。
     * 
     * @param index 数组索引（1-based，必须 >= 1）
     * @return 索引对应的值，如果索引超出范围返回nil
     */
    Value getArray(i32 index) const;
    
    /**
     * @brief 设置数组元素
     * 
     * Lua数组使用1-based索引。如果索引超出当前数组大小，
     * 会自动扩展数组（中间的空位填充nil）。
     * 
     * @param index 数组索引（1-based，必须 >= 1）
     * @param value 要设置的值
     */
    void setArray(i32 index, const Value& value);
    
    /**
     * @brief 获取数组部分的大小
     * 
     * @return 数组部分的元素数量
     */
    usize getArraySize() const noexcept {
        return array_.size();
    }
    
    /**
     * @brief 获取表的长度（Lua的#运算符）
     *
     * 返回数组部分中最后一个非nil值的索引。
     * 注意：这是简化实现，完整的Lua长度语义更复杂。
     *
     * @return 表的长度
     */
    usize length() const;

    // =====================================================================
    // 迭代器支持
    // =====================================================================

    /**
     * @brief 获取表中的下一个键值对（用于泛型for循环）
     *
     * 这是实现Lua的next()函数和pairs()迭代器的核心方法。
     * 遍历顺序：先遍历数组部分（索引1, 2, 3, ...），再遍历哈希部分。
     *
     * @param key 当前键（nil表示从头开始）
     * @param nextKey 输出参数，存储下一个键
     * @param nextValue 输出参数，存储下一个值
     * @return true 如果找到下一个键值对，false 如果已到表尾
     *
     * @note 遍历过程中修改表可能导致未定义行为
     * @see luaH_next() in lua_c_analysis/src/ltable.c
     */
    bool next(const Value& key, Value& nextKey, Value& nextValue) const;

    // =====================================================================
    // 元表操作
    // =====================================================================
    
    /**
     * @brief 获取元表
     * 
     * @return 元表指针，如果没有元表返回nullptr
     */
    Table* getMetatable() const noexcept {
        return metatable_;
    }
    
    /**
     * @brief 设置元表
     *
     * @param mt 元表指针，可以为nullptr（表示移除元表）
     */
    void setMetatable(Table* mt) noexcept {
        metatable_ = mt;
    }

    /**
     * @brief 获取元方法缓存标志位
     *
     * @return 标志位值
     */
    u8 getFlags() const noexcept {
        return flags_;
    }

    /**
     * @brief 设置元方法缓存标志位
     *
     * @param flags 新的标志位值
     */
    void setFlags(u8 flags) noexcept {
        flags_ = flags;
    }

    // =====================================================================
    // GCObject接口实现
    // =====================================================================
    
    /**
     * @brief 标记表中引用的所有GC对象
     * 
     * 遍历数组部分和哈希部分，标记所有引用的GC对象：
     * - 字符串对象
     * - 表对象
     * - 函数对象
     * - 用户数据对象
     * - 线程对象
     * - 元表
     */
    void mark(GarbageCollector& gc) override;

    /**
     * @brief 按弱表模式标记表内容
     *
     * 元表始终是强引用；数组元素在弱值模式下不标记；哈希键/值分别按弱键/弱值模式跳过。
     * 由 GarbageCollector::markTable 调用，普通代码不需要直接使用。
     */
    void markContents(GarbageCollector& gc, bool weakKeys, bool weakValues);

    /**
     * @brief 清理弱表中指向死亡对象的条目
     *
     * 该方法必须在 sweep 删除白色对象之前调用，确保仍可安全检查键和值的颜色。
     */
    void removeWeakEntries(const GarbageCollector& gc, bool weakKeys, bool weakValues);
    
    /**
     * @brief 获取表占用的内存大小
     * 
     * 包括：
     * - Table对象本身的大小
     * - 数组部分的容量
     * - 哈希部分的容量（估算）
     * 
     * @return 表占用的字节数
     */
    usize getSize() const override;
    
    // =====================================================================
    // 调试和统计
    // =====================================================================
    
    /**
     * @brief 获取哈希部分的大小
     * 
     * @return 哈希部分的元素数量
     */
    usize getHashSize() const noexcept {
        return hash_.size();
    }
    
    /**
     * @brief 获取表的总元素数量
     * 
     * @return 数组部分 + 哈希部分的元素总数
     */
    usize getTotalSize() const noexcept {
        return array_.size() + hash_.size();
    }

private:
    // =====================================================================
    // 内部数据成员
    // =====================================================================
    
    /// 数组部分：存储连续的正整数键（索引从1开始）
    Vec<Value> array_;

    /// 哈希部分：存储其他类型的键或非连续的整数键
    /// 注意：std::unordered_map需要4个模板参数：Key, Value, Hash, KeyEqual
    std::unordered_map<Value, Value, ValueHash, ValueEqual> hash_;

    /// 元表指针：用于元编程
    Table* metatable_;

    /// 元方法缓存标志位：用于快速判断元方法是否存在
    /// 每个位对应一个元方法类型（TM_INDEX到TM_EQ）
    /// 位为1表示该元方法不存在，避免重复查找
    /// @see lua_c_analysis/src/lobject.h Table结构的flags字段
    u8 flags_;

    // =====================================================================
    // 内部辅助方法
    // =====================================================================
    
    /**
     * @brief 检查键是否是有效的数组索引
     * 
     * 有效的数组索引必须满足：
     * 1. 是数字类型
     * 2. 是正整数
     * 3. 在合理的范围内（避免过大的索引导致内存问题）
     * 
     * @param key 要检查的键
     * @param outIndex 输出参数，如果是有效索引则存储索引值
     * @return true 如果是有效的数组索引
     */
    bool isArrayIndex(const Value& key, i32& outIndex) const;
};

} // namespace Lua

