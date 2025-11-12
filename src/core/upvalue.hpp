/**
 * @file upvalue.hpp
 * @brief Upvalue类：实现闭包的上值（捕获的外部变量）
 * 
 * 详细说明：
 * Upvalue是Lua实现闭包的关键数据结构。当内部函数引用外部函数的局部变量时，
 * 这些变量会被"提升"为上值，从而在外部函数返回后仍然保持可访问性。
 * 
 * 核心概念：
 * 1. **Open状态**：上值指向栈上的活跃变量
 *    - v_指针指向栈上的Value
 *    - stackIndex_记录栈索引位置
 *    - 在LuaState的openUpvalues_链表中
 * 
 * 2. **Closed状态**：上值拥有变量的独立副本
 *    - v_指针指向closedValue_
 *    - 变量已从栈中移除
 *    - 不在openUpvalues_链表中
 * 
 * 状态转换：
 * Open → Closed：当外部函数返回，栈上的变量被销毁时
 * 
 * 共享机制：
 * 多个闭包可以共享同一个Upvalue，确保变量修改的一致性
 * 
 * 设计特点：
 * - 继承GCObject：完全集成到垃圾回收系统
 * - 使用指针v_：统一处理open和closed状态
 * - 链表管理：支持高效的查找和批量关闭
 * - RAII资源管理：自动处理状态转换
 * 
 * @author Lua C++ Implementation
 * @date 2025-01-12
 */

#pragma once

#include "common/types.hpp"
#include "core/gc_object.hpp"
#include "core/value.hpp"

namespace Lua {

/**
 * @brief Upvalue类：闭包捕获的外部变量
 * 
 * 实现细节：
 * - Open状态：v_指向栈上的Value，stackIndex_有效
 * - Closed状态：v_指向closedValue_，stackIndex_无效
 * - 链表指针next_用于LuaState中的open upvalue链表
 */
class Upvalue : public GCObject {
public:
    /**
     * @brief 创建Open状态的Upvalue（指向栈上的值）
     * @param stackValue 指向栈上Value的指针
     * @param stackIndex 栈索引位置
     * @return 新创建的Upvalue指针
     * 
     * 使用场景：
     * - 闭包创建时捕获外部变量
     * - LuaState::findOrCreateUpvalue()中
     */
    static Upvalue* createOpen(Value* stackValue, usize stackIndex);
    
    /**
     * @brief 创建Closed状态的Upvalue（独立存储值）
     * @param value 要存储的值
     * @return 新创建的Upvalue指针
     * 
     * 使用场景：
     * - 测试代码
     * - 特殊情况下的upvalue创建
     */
    static Upvalue* createClosed(const Value& value);
    
    /**
     * @brief 析构函数
     * 
     * 注意：Upvalue由GC管理，不要手动delete
     */
    ~Upvalue() override = default;
    
    // ========== 状态查询 ==========
    
    /**
     * @brief 检查是否为Open状态
     * @return true表示指向栈上的值，false表示已关闭
     */
    bool isOpen() const noexcept;
    
    /**
     * @brief 检查是否为Closed状态
     * @return true表示已关闭，false表示指向栈上的值
     */
    bool isClosed() const noexcept;
    
    // ========== 值访问 ==========
    
    /**
     * @brief 获取Upvalue的值（可修改）
     * @return 值的引用
     * 
     * 注意：
     * - Open状态：返回栈上的值
     * - Closed状态：返回closedValue_
     */
    Value& getValue() noexcept;
    
    /**
     * @brief 获取Upvalue的值（只读）
     * @return 值的常量引用
     */
    const Value& getValue() const noexcept;
    
    // ========== 状态转换 ==========
    
    /**
     * @brief 关闭Upvalue（从Open转换为Closed）
     * 
     * 操作：
     * 1. 将栈上的值复制到closedValue_
     * 2. 更新v_指针指向closedValue_
     * 3. stackIndex_变为无效
     * 
     * 调用时机：
     * - 函数返回时（LuaState::closeUpvalues）
     * - 栈收缩时
     */
    void close();
    
    // ========== 栈索引管理 ==========
    
    /**
     * @brief 获取栈索引（仅Open状态有效）
     * @return 栈索引位置
     * 
     * 注意：Closed状态下返回值无意义
     */
    usize getStackIndex() const noexcept;
    
    // ========== 链表管理 ==========
    
    /**
     * @brief 获取链表中的下一个Upvalue
     * @return 下一个Upvalue指针，nullptr表示链表末尾
     */
    Upvalue* getNext() const noexcept;
    
    /**
     * @brief 设置链表中的下一个Upvalue
     * @param next 下一个Upvalue指针
     */
    void setNext(Upvalue* next) noexcept;

    // ========== GC支持 ==========

    /**
     * @brief 标记Upvalue及其引用的对象
     *
     * 标记策略：
     * - Open状态：标记自身即可（栈上的值由栈管理）
     * - Closed状态：标记自身和closedValue_中的GC对象
     */
    void mark() override;

    /**
     * @brief 获取Upvalue对象的大小
     * @return 对象占用的字节数
     */
    usize getSize() const override;

private:
    /**
     * @brief 私有构造函数（Open状态）
     * @param stackValue 指向栈上Value的指针
     * @param stackIndex 栈索引位置
     */
    Upvalue(Value* stackValue, usize stackIndex);

    /**
     * @brief 私有构造函数（Closed状态）
     * @param value 要存储的值
     */
    explicit Upvalue(const Value& value);

    // ========== 成员变量 ==========

    /**
     * @brief 值指针：指向当前值的位置
     *
     * 状态区分：
     * - Open状态：v_ == stackValue（指向栈上的Value）
     * - Closed状态：v_ == &closedValue_（指向内部存储）
     */
    Value* v_;

    /**
     * @brief 栈索引：记录栈上的位置（仅Open状态有效）
     *
     * 用途：
     * - LuaState中的upvalue链表按stackIndex_降序排列
     * - 用于查找和批量关闭upvalue
     */
    usize stackIndex_;

    /**
     * @brief 关闭状态时存储的值
     *
     * 生命周期：
     * - Open状态：未使用
     * - Closed状态：存储从栈复制的值
     */
    Value closedValue_;

    /**
     * @brief 链表指针：指向下一个Upvalue
     *
     * 用途：
     * - LuaState中的openUpvalues_链表
     * - 按stackIndex_降序排列
     */
    Upvalue* next_;
};

} // namespace Lua

