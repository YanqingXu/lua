/**
 * @file jump_patcher.cpp
 * @brief 跳转列表与回填辅助逻辑的实现
 */

#include "compiler/codegen/jump_patcher.hpp"

#include <stdexcept>

namespace Lua {

JumpPatcher::JumpPatcher(CodegenState& state) noexcept
    : state_(state) {}

/**
 * @brief 将未解析的 JMP 指令编码为单向链表。
 *
 * 跳转待决时，其 sBx 指向下一条未解析跳转的程序计数器，而非最终目标。patchList() 必须先
 * 读取下一节点，再用真实目标覆盖 sBx。blockManager.jpc_ 保存应落到“下一条普通指令”的跳转；
 * 发射字节码时会刷新该待决列表。
 */

i32 JumpPatcher::emitJump() {
    i32 pending = state_.blockManager.jpc_;
    state_.blockManager.jpc_ = NO_JUMP;
    i32 jumpPc = state_.bytecode.emitAsBx(state_.currentLine, OpCode::JMP, 0, NO_JUMP);
    concatJumpList(jumpPc, pending);
    return jumpPc;
}

i32 JumpPatcher::emitConditionalJump(OpCode op, i32 a, i32 b, i32 c) {
    if (op == OpCode::TESTSET && a == NO_REG) {
        op = OpCode::TEST;
        a = b;
        b = 0;
    }

    flushPendingJumps();
    state_.bytecode.emitABC(state_.currentLine, op, a, b, c);
    return emitJump();
}

void JumpPatcher::patchList(i32 list, i32 target) {
    while (list != NO_JUMP) {
        i32 next = getJump(list);
        fixJump(list, target);
        list = next;
    }
}

void JumpPatcher::patchList(const PatchList& list, i32 target) {
    for (i32 pc : list.pcs) {
        fixJump(pc, target);
    }
}

void JumpPatcher::patchToHere(i32 list) {
    state_.pc = state_.bytecode.instructionCount();
    concatJumpList(state_.blockManager.jpc_, list);
}

void JumpPatcher::patchToHere(const PatchList& list) {
    state_.pc = state_.bytecode.instructionCount();
    patchList(list, state_.pc);
}

void JumpPatcher::flushPendingJumps() {
    i32 target = state_.bytecode.instructionCount();
    patchList(state_.blockManager.jpc_, target);
    state_.blockManager.jpc_ = NO_JUMP;
}

void JumpPatcher::concatJumpList(i32& left, i32 right) {
    if (right == NO_JUMP) {
        return;
    }

    if (left == NO_JUMP) {
        left = right;
        return;
    }

    i32 list = left;
    i32 next = NO_JUMP;
    while ((next = getJump(list)) != NO_JUMP) {
        list = next;
    }
    fixJump(list, right);
}

i32 JumpPatcher::getLabel() const {
    return state_.bytecode.instructionCount();
}

void JumpPatcher::syncPc() {
    state_.pc = state_.bytecode.instructionCount();
}

i32 JumpPatcher::getJump(i32 pc) const {
    Instruction inst = state_.bytecode.instruction(pc);
    i32 offset = GETARG_sBx(inst);
    if (offset == NO_JUMP) {
        return NO_JUMP;
    }
    return (pc + 1) + offset;
}

void JumpPatcher::fixJump(i32 pc, i32 dest) {
    Instruction jump = state_.bytecode.instruction(pc);
    i32 offset = dest - (pc + 1);
    if (offset > MAXARG_sBx || offset < -MAXARG_sBx) {
        throw std::runtime_error("control structure too long");
    }
    SETARG_sBx(jump, offset);
    state_.bytecode.replaceInstruction(pc, jump);
}

PatchList JumpPatcher::collectPatchList(i32 list) const {
    PatchList result;

    while (list != NO_JUMP) {
        result.append(list);
        list = getJump(list);
    }

    return result;
}

}  // namespace Lua
