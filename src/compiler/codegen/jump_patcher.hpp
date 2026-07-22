#pragma once

/**
 * @file jump_patcher.hpp
 * @brief 代码生成器的跳转列表与回填边界
 */

#include "compiler/codegen/codegen_state.hpp"

namespace Lua {

/**
 * @brief 负责代码生成器的跳转列表与回填操作
 *
 * 跳转修补器保持原有代码生成器跳转语义不变，并将链表编码、待决 `jpc_` 处理与程序
 * 计数器偏移写入收拢到专用辅助对象中。它不拥有代码生成状态；降级期间外观与发射器继续
 * 共享同一可变状态。
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
