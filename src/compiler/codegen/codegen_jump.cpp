/**
 * @file codegen_jump.cpp
 * @brief CodeGenerator jump list and condition materialization helpers.
 */

#include "compiler/codegen/codegen.hpp"

#include <stdexcept>
#include <utility>

namespace Lua {

// =============================================================================
// Jump backpatching model
// =============================================================================
//
// 控制流生成时经常还不知道最终目标 PC：`if` 的出口、`break` 的出口、
// `while` 的失败分支都只能先占一个 `JMP sBx = NO_JUMP`，等目标位置
// 出现后再回填。这里同时保留两种表示：
//
// 1. 旧式 i32 jump list：把尚未回填的 `JMP` 当作单向链表节点。
//    节点的 `sBx` 在未最终回填前暂存 nextPc - (pc + 1)，
//    getjump() 解码回“下一条待回填 JMP 的 PC”；`NO_JUMP` 表示链表结束。
//
//        list head
//            |
//            v
//        pc 12: JMP -> pc 20 -> pc 34 -> NO_JUMP
//
//    patchList(list, target) 回填时必须先读 next，再覆盖当前 `sBx`：
//
//        pc 12: JMP target
//        pc 20: JMP target
//        pc 34: JMP target
//
// 2. PatchList：显式保存 PC 列表，用于 CondResult 的 trueList/falseList。
//    这比把布尔条件组合编码进隐式链表更直观，适合 `and` / `or` /
//    `not` 的短路组合；最终仍然通过 fixjump(pc, target) 写回真实偏移。
//
// `state_.blocks.jpc_` 表示“已经决定跳到当前位置，但当前位置还没有发出
// 实际指令”的延迟列表。下一条普通指令发出前会被 flushPendingJumps()
// 回填到当前 instructionCount()；如果当前位置马上又发出一个新的 JMP，
// jump() 会把 jpc_ 串到新 JMP 后面，让所有跳转最终直达同一个目标，
// 避免生成“跳到一条跳转”的中间跳。
//
// Helper contracts:
// - jump() drains jpc_ and links pending jumps behind the newly emitted JMP.
// - patchtohere(i32) records an old-style list in jpc_ instead of patching
//   immediately, so a following JMP can be folded into the same final target.
// - concatJumpList() finds the old-style list tail and temporarily rewrites it
//   to point at the appended list.
// - condjump() lowers TESTSET with NO_REG to TEST because NO_REG is a sentinel,
//   not a valid runtime register; the original B operand becomes TEST.A.
// - getjump() and fixjump() both use the VM encoding `target = pc + 1 + sBx`;
//   while unresolved, that target is the next list node, and after patching it
//   is the real runtime destination.

i32 CodeGenerator::jump() {
    return jumps_.emitJump();
}

void CodeGenerator::patchList(i32 list, i32 target) {
    jumps_.patchList(list, target);
}

void CodeGenerator::patchList(const PatchList& list, i32 target) {
    jumps_.patchList(list, target);
}

void CodeGenerator::flushPendingJumps() {
    jumps_.flushPendingJumps();
}

i32 CodeGenerator::getLabel() {
    return jumps_.getLabel();
}

void CodeGenerator::concatJumpList(i32& l1, i32 l2) {
    jumps_.concatJumpList(l1, l2);
}

i32 CodeGenerator::condjump(OpCode op, i32 a, i32 b, i32 c) {
    return jumps_.emitConditionalJump(op, a, b, c);
}

void CodeGenerator::patchtohere(i32 list) {
    jumps_.patchToHere(list);
}

void CodeGenerator::patchtohere(const PatchList& list) {
    jumps_.patchToHere(list);
}

void CodeGenerator::syncPC() {
    jumps_.syncPc();
}

i32 CodeGenerator::getjump(i32 pc) {
    return jumps_.getJump(pc);
}

void CodeGenerator::fixjump(i32 pc, i32 dest) {
    jumps_.fixJump(pc, dest);
}

PatchList CodeGenerator::collectPatchList(i32 list) {
    return jumps_.collectPatchList(list);
}


PatchList CodeGenerator::emitComparisonJump(const BinaryExpr& e, bool jumpOnTrue) {
    return expressions_.emitComparisonJump(e, jumpOnTrue);
}

void CodeGenerator::materializeCondResult(const CondResult& cond, i32 reg, bool fallthroughOnTrue) {
    expressions_.materializeCondResult(cond, reg, fallthroughOnTrue);
}

}  // namespace Lua
