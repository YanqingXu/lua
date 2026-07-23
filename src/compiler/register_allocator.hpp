#pragma once

/**
 * @file register_allocator.hpp
 * @brief 代码生成阶段的虚拟寄存器分配器
 */

#include "common/types.hpp"
#include "core/function.hpp"

namespace Lua {

/**
 * @brief 寄存器分配器
 *
 * 从代码生成器中提取的寄存器管理子系统。
 * 负责临时寄存器的分配与回收，以及最大栈容量的维护。
 *
 * @note 代码生成器通过语义化方法移动或恢复空闲寄存器指针，并提供寄存器分配、回收和栈容量检查方法。
 */
class RegisterAllocator {
public:
    RegisterAllocator() = default;

    /**
     * @brief 绑定当前函数原型。
     * @param proto 当前正在编译的函数原型。
     */
    void bind(Proto* proto) noexcept {
        proto_ = proto;
    }

    /**
     * @brief 获取下一个空闲寄存器的位置。
     * @return 下一个空闲寄存器的索引。
     */
    i32 current() const noexcept {
        return freereg_;
    }

    /**
     * @brief 分配一个新寄存器并更新最大栈容量。
     * @return 新寄存器的索引。
     */
    i32 alloc() {
        i32 reg = freereg_++;
        if (freereg_ > static_cast<i32>(proto_->getMaxStackSize())) {
            proto_->setMaxStackSize(static_cast<u8>(freereg_));
        }
        return reg;
    }

    /**
     * @brief 尝试回收指定寄存器。
     * @param reg 待回收的寄存器索引。
     * @param activeLocals 当前活动局部变量数量。
     */
    void freeReg(i32 reg, i32 activeLocals) {
        if (reg >= activeLocals && reg == freereg_ - 1) {
            freereg_--;
        }
    }

    /**
     * @brief 回收栈顶指定数量的寄存器。
     * @param n 待回收的寄存器数量。
     */
    void freeRegs(i32 n) {
        for (i32 i = 0; i < n; i++) {
            freereg_--;
        }
    }

    /**
     * @brief 检查并更新最大栈容量。
     * @param n 需要额外容纳的寄存器数量。
     */
    void checkStack(i32 n) {
        i32 newstack = freereg_ + n;
        if (newstack > static_cast<i32>(proto_->getMaxStackSize())) {
            proto_->setMaxStackSize(static_cast<u8>(newstack));
        }
    }

    /**
     * @brief 将下一个空闲寄存器设置到指定位置。
     * @param reg 新的空闲寄存器索引。
     */
    void setFreeReg(i32 reg) noexcept {
        freereg_ = reg;
    }

    /**
     * @brief 将下一个空闲寄存器重置到活动局部变量之后。
     * @param activeLocals 当前活动局部变量数量。
     */
    void resetToLocals(i32 activeLocals) noexcept {
        freereg_ = activeLocals;
    }

    /**
     * @brief 恢复到先前保存的空闲寄存器位置。
     * @param saved 先前保存的位置。
     */
    void restore(i32 saved) noexcept {
        freereg_ = saved;
    }

    /**
     * @brief 预留连续寄存器，但不立即更新最大栈容量。
     * @param count 待预留的寄存器数量。
     */
    void reserve(i32 count) noexcept {
        freereg_ += count;
    }

    /**
     * @brief 确保下一个空闲寄存器至少位于指定位置。
     * @param reg 最小寄存器索引。
     */
    void ensureAtLeast(i32 reg) noexcept {
        if (freereg_ < reg) {
            freereg_ = reg;
        }
    }

    /**
     * @brief 重置到子函数编译所需的初始状态。
     * @param start 初始空闲寄存器索引。
     */
    void reset(i32 start = 0) noexcept {
        freereg_ = start;
    }

private:
    Proto* proto_ = nullptr;
    i32 freereg_ = 0;
};

} // namespace Lua
