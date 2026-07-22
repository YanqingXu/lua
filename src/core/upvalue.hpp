/**
 * @file upvalue.hpp
 * @brief 上值类：实现闭包捕获的外部变量
 *
 * 详细说明：
 * 上值是 Lua 实现闭包的关键数据结构。当内部函数引用外部函数的局部变量时，
 * 这些变量会被"提升"为上值，从而在外部函数返回后仍然保持可访问性。
 *
 * 核心概念：
 * 1. **开放状态**：上值指向栈上的活跃变量
 *    - v_ 指针指向栈上的值
 *    - stackIndex_记录栈索引位置
 *    - 位于 Lua 状态的开放上值链表中
 *
 * 2. **关闭状态**：上值拥有变量的独立副本
 *    - v_ 指针指向 closedValue_
 *    - 变量已从栈中移除
 *    - 不在开放上值链表中
 *
 * 状态转换：
 * 开放 → 关闭：当外部函数返回，栈上的变量被销毁时
 *
 * 共享机制：
 * 多个闭包可以共享同一个上值，确保变量修改的一致性
 *
 * 设计特点：
 * - 继承垃圾回收对象：完全集成到垃圾回收系统
 * - 使用指针 v_：统一处理开放和关闭状态
 * - 链表管理：支持高效的查找和批量关闭
 * - 资源获取即初始化管理：自动处理状态转换
 *
 * @author Lua C++ 实现团队
 * @date 2025-01-12
 */

#pragma once

#include "common/types.hpp"
#include "core/gc_object.hpp"
#include "core/value.hpp"

namespace Lua {

// 前向声明
class Stack;

/**
 * @brief 上值类：闭包捕获的外部变量
 *
 * 实现细节（✅ 改进版 - 使用索引避免悬空指针）：
 * - 开放状态：只存储 stackIndex_，通过栈动态获取值
 * - 关闭状态：存储在 closedValue_ 中
 * - 链表指针 next_ 用于 Lua 状态中的开放上值链表
 *
 * ✅ 改进说明：
 * 原实现使用指针 v_ 指向栈上的值，当动态数组调整容量时会导致悬空指针。
 * 改进后只存储索引，每次访问时动态计算地址，完全避免悬空指针问题。
 */
class Upvalue : public GCObject {
public:
    /**
     * @brief 创建开放状态的上值（指向栈上的值）
     * @param stackIndex 栈索引位置
     * @return 新创建的上值指针
     *
     * ✅ 改进：只传递索引，不传递指针
     *
     * 使用场景：
     * - 闭包创建时捕获外部变量
     * - Lua 状态查找或创建上值时
     */
    static Upvalue* createOpen(usize stackIndex, Stack& ownerStack);

    /**
     * @brief 创建关闭状态的上值（独立存储值）
     * @param value 要存储的值
     * @return 新创建的上值指针
     *
     * 使用场景：
     * - 测试代码
     * - 特殊情况下创建上值
     */
    static Upvalue* createClosed(const Value& value);

    /**
     * @brief 析构函数
     *
     * @note 上值由垃圾回收器管理，不要手动释放。
     */
    ~Upvalue() override = default;

    // ========== 状态查询 ==========

    /**
     * @brief 检查是否为开放状态
     * @return true 表示指向栈上的值，false 表示已关闭
     */
    bool isOpen() const noexcept;

    /**
     * @brief 检查是否为关闭状态
     * @return true表示已关闭，false表示指向栈上的值
     */
    bool isClosed() const noexcept;

    // ========== 值访问（✅ 改进版 - 需要传入Stack引用） ==========

    /**
     * @brief 获取上值的可修改值
     * @param stack 栈引用
     * @return 值的引用
     *
     * 改进：需要传入栈引用，动态计算地址
     *
     * 注意：
     * - 开放状态：返回栈上的值（通过索引动态获取）
     * - 关闭状态：返回 closedValue_
     */
    Value& getValue(Stack& stack) noexcept;

    /**
     * @brief 获取上值的只读值
     * @param stack 栈引用
     * @return 值的常量引用
     */
    const Value& getValue(const Stack& stack) const noexcept;

    /**
     * @brief 设置上值
     * @param stack 栈引用
     * @param value 新值
     *
     * ✅ 新增：提供设置值的接口
     */
    void setValue(Stack& stack, const Value& value);

    // ========== 状态转换 ==========

    /**
     * @brief 关闭上值（从开放状态转换为关闭状态）
     * @param stack 栈引用
     *
     * 改进：需要传入栈引用以复制值
     *
     * 操作：
     * 1. 将栈上的值复制到closedValue_
     * 2. 标记为关闭状态
     * 3. stackIndex_保持不变（用于调试）
     *
     * 调用时机：
     * - 函数返回时（LuaState::closeUpvalues）
     * - 栈收缩时
     */
    void close(Stack& stack) noexcept;

    // ========== 栈索引管理 ==========

    /**
     * @brief 获取栈索引（仅开放状态有效）
     * @return 栈索引位置
     *
     * @note 关闭状态下返回值无意义。
     */
    usize getStackIndex() const noexcept;

    // ========== 链表管理 ==========

    /**
     * @brief 获取链表中的下一个上值
     * @return 下一个上值指针，空指针表示链表末尾
     */
    Upvalue* getNext() const noexcept;

    /**
     * @brief 设置链表中的下一个上值
     * @param next 下一个上值指针
     */
    void setNext(Upvalue* next) noexcept;

    // ========== GC支持 ==========

    /**
     * @brief 标记上值及其引用的对象
     *
     * 标记策略：
     * - 开放状态：标记自身即可（栈上的值由栈管理）
     * - 关闭状态：标记自身和 closedValue_ 中的垃圾回收对象
     */
    void mark(GarbageCollector& gc) override;

    /**
     * @brief 获取上值对象的大小
     * @return 对象占用的字节数
     */
    usize getSize() const override;

    /**
     * @brief 垃圾回收工厂构造函数（开放状态）
     * @param stackIndex 栈索引位置
     *
     * ✅
     * 改进：只接受索引参数
     */
    Upvalue(usize stackIndex, Stack& ownerStack);

    /**
     * @brief 垃圾回收工厂构造函数（关闭状态）
     * @param value 要存储的值
     */
    explicit Upvalue(const Value& value);

private:
    // ========== 成员变量（✅ 改进版） ==========

    /**
     * @brief 是否为开放状态
     *
     * ✅ 新增：显式标记状态，避免通过指针判断
     *
     * - true：开放状态，使用 stackIndex_
     * - false：关闭状态，使用 closedValue_
     */
    bool isOpen_;

    /**
     * @brief 栈索引：记录栈上的位置
     *
     * ✅ 改进：始终有效，用于动态获取栈上的值
     *
     * 用途：
     * - 开放状态：通过此索引访问栈上的值
     * - 关闭状态：保持不变（用于调试）
     * - Lua 状态中的上值链表按 stackIndex_ 降序排列
     */
    usize stackIndex_;

    /**
     * @brief 关闭状态时存储的值
     *
     * 生命周期：
     * - 开放状态：未使用
     * - 关闭状态：存储从栈复制的值
     */
    Value closedValue_;

    /**
     * @brief 链表指针：指向下一个上值
     *
     * 用途：
     * - Lua 状态中的开放上值链表
     * - 按 stackIndex_ 降序排列
     */
    Upvalue* next_;

    /**
     * @brief 所属栈指针（开放状态）
     *
     * 用于跨协程时正确访问上值所在的栈。
     * 开放状态：指向创建时的栈
     * 关闭状态：不使用
     */
    Stack* ownerStack_;
};

} // namespace Lua
