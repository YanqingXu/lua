#pragma once

/**
 * @file scope_manager.hpp
 * @brief 代码生成器的局部变量、代码块与上值作用域边界
 */

#include "compiler/codegen/jump_patcher.hpp"

namespace Lua {

/**
 * @brief 负责代码生成器的作用域生命周期操作
 *
 * 作用域管理器集中处理局部变量激活与移除、代码块栈记账、break 列表回填、上值注册与 CLOSE
 * 发射。它与外观和其他发射器共享代码生成状态，但不拥有该状态。
 */
class ScopeManager {
public:
    ScopeManager(CodegenState& state, JumpPatcher& jumps) noexcept;

    i32 addLocalVar(const Str& name);
    [[nodiscard]] i32 findLocalVar(const Str& name) const;
    void markLocalCaptured(i32 reg);
    void adjustLocalVars(i32 count);
    void removeLocalVars(i32 toLevel);
    void closeScopeUpvalues(i32 level);

    [[nodiscard]] i32 activeLocalCount() const noexcept;
    [[nodiscard]] const Vec<LocalVar>& localVars() const noexcept;

    i32 findUpvalue(const Str& name) const;
    i32 addUpvalue(const Str& name, bool inStack, i32 index);
    i32 resolveUpvalue(const Str& name);
    [[nodiscard]] const Vec<UpvalueCapture>& upvalues() const noexcept;

    void enterBlock(bool isBreakable);
    void leaveBlock();
    [[nodiscard]] BlockInfo* currentBlock() const noexcept;
    [[nodiscard]] BlockInfo* findBreakableBlock() const noexcept;
    void appendBreakJump(BlockInfo& block, i32 jumpPc);

private:
    void emitClose(i32 level);

    CodegenState& state_;
    JumpPatcher& jumps_;
};

}  // namespace Lua
