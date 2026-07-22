/**
 * @file stack.hpp
 * @brief Lua栈管理：动态扩展的值栈实现
 *
 * 详细说明：
 * 栈类管理 Lua 虚拟机的值栈，用于存储函数参数、局部变量和临时值。
 * 栈采用连续内存布局，支持高效的随机访问和动态扩展。
 *
 * 核心特性：
 * - 动态扩展：栈空间不足时自动扩展
 * - 边界检查：防止栈溢出和下溢
 * - 高效访问：常数时间复杂度的压栈和出栈操作
 * - 内存安全：使用动态数组管理内存并自动释放
 *
 * 栈布局：
 * ```
 * 高地址 ┌─────────────┐ ← capacity (栈容量)
 *       │             │
 *       │   可用空间   │
 *       │             │
 *       ├─────────────┤ ← top (栈顶)
 *       │   值 n      │
 *       │   ...       │
 *       │   值 1      │
 * 低地址 └─────────────┘ ← base (栈底，索引0)
 * ```
 * @author Lua C++ 项目
 * @date 2025-11-12
 */

#pragma once

#include "common/types.hpp"
#include "core/value.hpp"
#include "runtime/lua_allocator.hpp"
#include "runtime/resource_policy.hpp"
#include "vm/vm_constants.hpp"

namespace Lua {

/**
 * @brief 栈类
 *
 * 管理 Lua 值的动态栈，支持压栈、出栈和自动扩展。
 *
 * 使用示例：
 * @code
 * Stack stack;
 *
 * // 压入值
 * stack.push(Value(42.0));
 * stack.push(Value(true));
 *
 * // 访问栈顶
 * Value top = stack.top();
 *
 * // 弹出值
 * Value val = stack.pop();
 *
 * // 通过索引访问
 * Value v = stack.at(0);  // 访问栈底
 * @endcode
 */
class Stack {
public:
    // =====================================================================
    // 构造函数和析构函数
    // =====================================================================

    /**
     * @brief 构造函数
     * @param initialSize 初始栈大小（默认为INITIAL_STACK_SIZE）
     */
    explicit Stack(usize initialSize = INITIAL_STACK_SIZE, LuaAllocator* allocator = nullptr,
                   const ResourcePolicy* resourcePolicy = nullptr);

    /**
     * @brief 析构函数
     */
    ~Stack() = default;

    // 禁止拷贝，允许移动
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;
    Stack(Stack&&) noexcept = default;
    Stack& operator=(Stack&&) noexcept = default;

    // =====================================================================
    // 栈操作（✅ 改进版 - 添加预检查和快速push）
    // =====================================================================

    /**
     * @brief 检查栈空间是否足够（✅ 新增 - 预检查机制）
     * @param needed 需要的空间数量
     * @throws MemoryError 如果超过最大栈限制
     *
     * 用途：
     * - 批量操作前预先检查空间
     * - 避免每次压栈都检查（性能优化）
     *
     * 使用示例：
     * ```cpp
     * stack.checkSpace(100);  // 预先确保有100个空间
     * for (int i = 0; i < 100; i++) {
     *     stack.pushUnchecked(value);  // 快速push，无需检查
     * }
     * ```
     */
    void checkSpace(usize needed);

    /**
     * @brief 按当前资源策略检查绝对逻辑或物理栈顶
     * @param newTop 请求的绝对栈顶位置
     */
    void checkLimit(usize newTop) const;

    /**
     * @brief 将值压入栈顶（新增的无检查优化版本）
     * @param value 要压入的值
     *
     * 注意：
     * - 调用前必须确保有足够空间（使用checkSpace）
     * - 不进行边界检查，性能更高
     * - 仅用于性能关键路径
     */
    void pushUnchecked(const Value& value) noexcept;

    /**
     * @brief 将值压入栈顶（兼容版本）
     * @param value 要压入的值
     *
     * 注意：
     * - 自动检查空间并扩展
     * - 适用于单次压栈操作
     */
    void push(const Value& value);

    /**
     * @brief 弹出栈顶值
     * @return 栈顶的值
     * @throws RuntimeError 如果栈为空
     */
    Value pop();

    /**
     * @brief 获取栈顶值（不弹出）
     * @return 栈顶值的引用
     * @throws RuntimeError 如果栈为空
     */
    Value& top();
    const Value& top() const;

    /**
     * @brief 通过索引访问栈元素
     * @param index 索引（0为栈底，size()-1为栈顶）
     * @return 指定位置的值的引用
     * @throws std::out_of_range 如果索引越界
     */
    Value& at(usize index);
    const Value& at(usize index) const;

    /**
     * @brief 通过索引访问栈元素（不检查边界）
     * @param index 索引
     * @return 指定位置的值的引用
     */
    Value& operator[](usize index) noexcept {
        return stack_[index];
    }

    const Value& operator[](usize index) const noexcept {
        return stack_[index];
    }

    // =====================================================================
    // 栈状态查询
    // =====================================================================

    /**
     * @brief 获取栈中元素数量
     * @return 栈大小
     */
    usize size() const noexcept {
        return top_;
    }

    /**
     * @brief 获取栈容量
     * @return 栈容量
     */
    usize capacity() const noexcept {
        return stack_.size();
    }

    /**
     * @brief 检查栈是否为空
     * @return 如果栈为空返回true
     */
    bool empty() const noexcept {
        return top_ == 0;
    }

    /**
     * @brief 清空栈
     */
    void clear() noexcept {
        top_ = 0;
    }

    // =====================================================================
    // 栈空间管理
    // =====================================================================

    /**
     * @brief 确保栈有足够的空间
     * @param needed 需要的额外空间
     */
    void ensureSpace(usize needed);

    /**
     * @brief 设置栈顶位置
     * @param newTop 新的栈顶位置
     */
    void setTop(usize newTop);

private:
    /**
     * @brief 值栈（使用Vec自动管理内存）
     */
    LuaVector<Value> stack_;

    /**
     * @brief 栈顶位置（指向下一个可用位置）
     */
    usize top_;

    /** @brief 从所属全局状态借用；每个上下文仍可修改策略字段。 */
    const ResourcePolicy* resourcePolicy_;
};

} // namespace Lua
