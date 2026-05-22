#pragma once

/**
 * @file scope_manager.hpp
 * @brief Local, block, and upvalue scope boundary for CodeGenerator.
 */

#include "compiler/codegen/jump_patcher.hpp"

namespace Lua {

/**
 * @brief Owns CodeGenerator scope lifecycle operations.
 *
 * ScopeManager centralizes local variable activation/removal, block stack
 * bookkeeping, break-list patching, upvalue registration, and CLOSE emission.
 * It shares CodegenState with the facade and other emitters; it does not own
 * the state.
 */
class ScopeManager {
public:
    ScopeManager(CodegenState& state, JumpPatcher& jumps) noexcept;

    i32 addLocalVar(const Str& name);
    [[nodiscard]] i32 findLocalVar(const Str& name) const;
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
