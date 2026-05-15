#pragma once

#include "common/types.hpp"
#include "core/function.hpp"

namespace Lua {

/**
 * @brief 寄存器分配器
 *
 * 从 CodeGenerator 中提取的寄存器管理子系统。
 * 负责临时寄存器的分配/回收，以及 maxStackSize 的维护。
 *
 * 迁移说明:
 * - CodeGenerator 通过语义化方法移动/恢复空闲寄存器指针
 * - allocReg() / freeReg() / freeRegs() / checkStack() 作为便捷方法提供
 */
class RegisterAllocator {
public:
    RegisterAllocator() = default;

    /// 绑定当前 Proto（用于读写 maxStackSize）
    void bind(Proto* proto) noexcept { proto_ = proto; }

    /// 当前下一个空闲寄存器
    i32 current() const noexcept { return freereg_; }

    /// 分配一个新寄存器，更新 maxStackSize（原 CodeGenerator::allocReg）
    i32 alloc() {
        i32 reg = freereg_++;
        if (freereg_ > static_cast<i32>(proto_->getMaxStackSize())) {
            proto_->setMaxStackSize(static_cast<u8>(freereg_));
        }
        return reg;
    }

    /// 尝试回收寄存器（原 CodeGenerator::freeReg）
    void freeReg(i32 reg, i32 activeLocals) {
        if (reg >= activeLocals && reg == freereg_ - 1) {
            freereg_--;
        }
    }

    /// 回收栈顶 n 个寄存器（原 CodeGenerator::freeRegs）
    void freeRegs(i32 n) {
        for (i32 i = 0; i < n; i++) {
            freereg_--;
        }
    }

    /// 检查并更新 maxStackSize（原 CodeGenerator::checkStack）
    void checkStack(i32 n) {
        i32 newstack = freereg_ + n;
        if (newstack > static_cast<i32>(proto_->getMaxStackSize())) {
            proto_->setMaxStackSize(static_cast<u8>(newstack));
        }
    }

    /// 将下一个空闲寄存器设置到指定位置
    void setFreeReg(i32 reg) noexcept { freereg_ = reg; }

    /// 将下一个空闲寄存器重置到当前活动局部变量之后
    void resetToLocals(i32 activeLocals) noexcept { freereg_ = activeLocals; }

    /// 恢复到先前保存的空闲寄存器位置
    void restore(i32 saved) noexcept { freereg_ = saved; }

    /// 保留连续寄存器，不立即更新 maxStackSize
    void reserve(i32 count) noexcept { freereg_ += count; }

    /// 确保下一个空闲寄存器至少位于指定位置
    void ensureAtLeast(i32 reg) noexcept {
        if (freereg_ < reg) {
            freereg_ = reg;
        }
    }

    /// 重置到初始状态（用于子函数编译）
    void reset(i32 start = 0) noexcept { freereg_ = start; }

private:
    Proto* proto_ = nullptr;
    i32 freereg_ = 0;
};

}  // namespace Lua
