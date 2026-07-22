#pragma once

/**
 * @file codegen_context.hpp
 * @brief 代码生成器的局部变量、作用域与上值上下文类型
 */

#include "common/types.hpp"
#include "compiler/codegen/codegen_types.hpp"
#include "compiler/register_allocator.hpp"
#include <stdexcept>

namespace Lua {

class Proto;

// 前向声明
class GCString;

// =============================================================================
// LocalVar — 局部变量信息（从 codegen.hpp 移出）
// =============================================================================

/** @brief 代码生成期间跟踪的局部变量信息。 */
struct LocalVar {
    Str name;
    i32 reg;
    i32 startpc;
    i32 endpc;
    bool captured;

    LocalVar(const Str& n, i32 r, i32 start)
        : name(n), reg(r), startpc(start), endpc(-1), captured(false) {}
};

// =============================================================================
// LocalVarScope — 局部变量作用域管理器
// =============================================================================

/**
 * @brief 局部变量作用域管理器
 *
 * 从代码生成器中提取的局部变量管理子系统。
 * @note 第 7 次拉取请求的迁移说明：
 * - activeVarCount_  现在由 CodegenState::localScope.activeVarCount_ 持有（公开字段）
 * - localVars_ 现在由 CodegenState::localScope.localVars_ 持有（公开字段）
 */
class LocalVarScope {
public:
    LocalVarScope() = default;

    i32 findLocal(const Str& name) const {
        for (i32 i = static_cast<i32>(localVars_.size()) - 1; i >= 0; i--) {
            if (localVars_[i].name == name && localVars_[i].endpc == -1) {
                return localVars_[i].reg;
            }
        }
        return -1;
    }

    void markCaptured(i32 reg) {
        for (i32 i = static_cast<i32>(localVars_.size()) - 1; i >= 0; --i) {
            if (localVars_[i].reg == reg && localVars_[i].endpc == -1) {
                localVars_[i].captured = true;
                return;
            }
        }
    }

    bool hasCapturedLocalsFrom(i32 level) const {
        for (const LocalVar& local : localVars_) {
            if (local.endpc == -1 && local.reg >= level && local.captured) {
                return true;
            }
        }
        return false;
    }

    /** @brief 关闭离开作用域的局部变量并设置 endpc。 */
    void closeLocals(i32 tolevel, i32 currentPc) {
        while (activeVarCount_ > tolevel) {
            activeVarCount_--;
            for (i32 i = static_cast<i32>(localVars_.size()) - 1; i >= 0; --i) {
                if (localVars_[i].endpc == -1) {
                    localVars_[i].endpc = currentPc;
                    break;
                }
            }
        }
    }

    void clear() noexcept {
        localVars_.clear();
        activeVarCount_ = 0;
    }

    // === 公开字段（兼容旧代码的直接读写） ===
    Vec<LocalVar> localVars_;
    i32 activeVarCount_ = 0;
};

// =============================================================================
/** @brief 上值捕获信息与上值上下文。 */
// =============================================================================

/**
 * @brief 上值捕获信息
 */
struct UpvalueCapture {
    Str name;
    bool inStack;
    i32 index;

    UpvalueCapture(const Str& n, bool inStackVar, i32 idx)
        : name(n), inStack(inStackVar), index(idx) {}
};

/**
 * @brief 上值上下文
 *
 * 管理当前函数捕获的上值列表。
 * parent 指针保留在代码生成状态中，用于跨函数解析上值。
 *
 * @note 第 7 次拉取请求的迁移说明：
 * - upvalues_ 现在由 CodegenState::upvalueContext.upvalues_ 持有（公开字段）
 */
class UpvalueContext {
public:
    UpvalueContext() = default;

    i32 find(const Str& name) const {
        for (i32 i = 0; i < static_cast<i32>(upvalues_.size()); i++) {
            if (upvalues_[i].name == name) {
                return i;
            }
        }
        return -1;
    }

    i32 add(const Str& name, bool inStack, i32 index) {
        i32 existing = find(name);
        if (existing >= 0) {
            return existing;
        }
        upvalues_.emplace_back(name, inStack, index);
        return static_cast<i32>(upvalues_.size()) - 1;
    }

    void clear() noexcept { upvalues_.clear(); }

    // === 公开字段（兼容旧代码的直接读写） ===
    Vec<UpvalueCapture> upvalues_;
};

// =============================================================================
/** @brief 代码块信息与代码块管理器。 */
// =============================================================================

/**
 * @brief 代码块信息
 */
struct BlockInfo {
    BlockInfo* previous;
    UPtr<BlockInfo> previousOwner;
    i32 breaklist;
    i32 activeVarCount;
    bool isbreakable;

    BlockInfo(UPtr<BlockInfo> prev, i32 activeCount, bool breakable)
        : previous(prev.get())
        , previousOwner(std::move(prev))
        , breaklist(NO_JUMP)
        , activeVarCount(activeCount)
        , isbreakable(breakable) {}
};

/** @brief 已编译子函数原型及其上值捕获信息。 */
struct CompiledFunction {
    Proto* proto = nullptr;
    i32 protoIndex = -1;
    Vec<UpvalueCapture> upvalues;
};

/**
 * @brief 代码块与跳转管理器
 *
 * 从代码生成器中提取的代码块嵌套和跳转链管理子系统。
 *
 * @note 第 7 次拉取请求的迁移说明：
 * - currentBlock_ 现在由 CodegenState::blockManager.currentBlock_ 持有（公开字段）
 * - jpc_          现在由 CodegenState::blockManager.jpc_ 持有（公开字段）
 */
class BlockManager {
public:
    BlockManager() = default;

    void enterBlock(bool isBreakable, i32 activeVarCount) {
        currentBlockOwner_ = std::make_unique<BlockInfo>(
            std::move(currentBlockOwner_), activeVarCount, isBreakable);
        currentBlock_ = currentBlockOwner_.get();
    }

    [[nodiscard]] UPtr<BlockInfo> takeCurrentBlock() {
        if (currentBlock_ == nullptr) {
            throw std::runtime_error("No block to leave");
        }

        UPtr<BlockInfo> block = std::move(currentBlockOwner_);
        currentBlockOwner_ = std::move(block->previousOwner);
        currentBlock_ = currentBlockOwner_.get();
        block->previous = nullptr;
        return block;
    }

    /**
     * @brief 离开当前代码块、移除局部变量并修复 break 跳转
     * @param localScope 局部变量作用域
     * @param registers 寄存器分配器
     * @param currentPc 当前指令位置
     * @param patchToHere 将跳转回填到当前位置的回调
     */
    void leaveBlock(LocalVarScope& localScope, RegisterAllocator& registers,
                    i32 currentPc, const std::function<void(i32)>& patchToHere) {
        UPtr<BlockInfo> bl = takeCurrentBlock();

        localScope.closeLocals(bl->activeVarCount, currentPc);
        registers.resetToLocals(localScope.activeVarCount_);
        registers.checkStack(0);

        patchToHere(bl->breaklist);
    }

    void reset() {
        currentBlockOwner_.reset();
        currentBlock_ = nullptr;
        jpc_ = NO_JUMP;
    }

    // === 公开字段（兼容旧代码的直接读写） ===
    BlockInfo* currentBlock_ = nullptr;
    i32 jpc_ = NO_JUMP;

private:
    UPtr<BlockInfo> currentBlockOwner_;
};

}  // namespace Lua
