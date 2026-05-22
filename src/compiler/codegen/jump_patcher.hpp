#pragma once

/**
 * @file jump_patcher.hpp
 * @brief Jump list and backpatching boundary for CodeGenerator.
 */

#include "compiler/codegen/codegen_state.hpp"

namespace Lua {

/**
 * @brief Owns CodeGenerator jump-list and backpatching operations.
 *
 * JumpPatcher keeps the old CodeGenerator jump semantics intact while moving
 * the linked-list encoding, pending `jpc_` handling, and PC offset writes behind
 * a focused helper. It does not own CodegenState; the facade and emitters keep
 * sharing the same mutable state during lowering.
 */
class JumpPatcher {
public:
    explicit JumpPatcher(CodegenState& state) noexcept;

    i32 emitJump();
    i32 emitConditionalJump(OpCode op, i32 a, i32 b, i32 c);

    void patchList(i32 list, i32 target);
    void patchList(const PatchList& list, i32 target);
    void patchToHere(i32 list);
    void patchToHere(const PatchList& list);
    void flushPendingJumps();
    void concatJumpList(i32& left, i32 right);

    [[nodiscard]] i32 getLabel() const;
    void syncPc();

    [[nodiscard]] i32 getJump(i32 pc) const;
    void fixJump(i32 pc, i32 dest);
    [[nodiscard]] PatchList collectPatchList(i32 list) const;

private:
    CodegenState& state_;
};

}  // namespace Lua
