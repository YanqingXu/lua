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
 * 迁移说明 (PR-7):
 * - freereg_ 现在是 regs_.freereg_（公开字段，兼容旧代码）
 * - allocReg() / freeReg() / freeRegs() / checkStack() 作为便捷方法提供
 */
class RegisterAllocator {
public:
    RegisterAllocator() = default;

    /// 绑定当前 Proto（用于读写 maxStackSize）
    void bind(Proto* proto) noexcept { proto_ = proto; }

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

    /// 重置到初始状态（用于子函数编译）
    void reset(i32 start = 0) noexcept { freereg_ = start; }

    // === 公开字段（兼容旧代码的直接读写） ===
    i32 freereg_ = 0;

private:
    Proto* proto_ = nullptr;
};

}  // namespace Lua
