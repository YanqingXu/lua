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
    i32 jpc = state_.blocks.jpc_;
    state_.blocks.jpc_ = NO_JUMP;
    i32 j = codeAsBx(OpCode::JMP, 0, NO_JUMP);
    concatJumpList(j, jpc);
    return j;
}

void CodeGenerator::patchList(i32 list, i32 target) {
    while (list != NO_JUMP) {
        i32 next = getjump(list);
        fixjump(list, target);
        list = next;
    }
}

void CodeGenerator::patchList(const PatchList& list, i32 target) {
    for (i32 pc : list.pcs) {
        fixjump(pc, target);
    }
}

void CodeGenerator::flushPendingJumps() {
    i32 target = state_.bytecode.instructionCount();
    patchList(state_.blocks.jpc_, target);
    state_.blocks.jpc_ = NO_JUMP;
}

i32 CodeGenerator::getLabel() {
    return state_.bytecode.instructionCount();
}

void CodeGenerator::concatJumpList(i32& l1, i32 l2) {
    if (l2 == NO_JUMP) return;
    if (l1 == NO_JUMP) {
        l1 = l2;
    } else {
        i32 list = l1;
        i32 next;
        while ((next = getjump(list)) != NO_JUMP) {
            list = next;
        }
        fixjump(list, l2);
    }
}

i32 CodeGenerator::condjump(OpCode op, i32 a, i32 b, i32 c) {
    if (op == OpCode::TESTSET && a == NO_REG) {
        op = OpCode::TEST;
        a = b;
        b = 0;
    }

    codeABC(op, a, b, c);
    i32 jpc = state_.blocks.jpc_;
    state_.blocks.jpc_ = NO_JUMP;
    i32 j = codeAsBx(OpCode::JMP, 0, NO_JUMP);
    concatJumpList(j, jpc);
    return j;
}

void CodeGenerator::patchtohere(i32 list) {
    state_.pc = state_.bytecode.instructionCount();
    concatJumpList(state_.blocks.jpc_, list);
}

void CodeGenerator::patchtohere(const PatchList& list) {
    state_.pc = state_.bytecode.instructionCount();
    patchList(list, state_.pc);
}

void CodeGenerator::syncPC() {
    state_.pc = state_.bytecode.instructionCount();
}

i32 CodeGenerator::getjump(i32 pc) {
    Instruction inst = state_.bytecode.instruction(pc);
    i32 offset = GETARG_sBx(inst);
    if (offset == NO_JUMP) {
        return NO_JUMP;
    } else {
        return (pc + 1) + offset;
    }
}

void CodeGenerator::fixjump(i32 pc, i32 dest) {
    Instruction jmp = state_.bytecode.instruction(pc);
    i32 offset = dest - (pc + 1);
    if (offset > MAXARG_sBx || offset < -MAXARG_sBx) {
        throw std::runtime_error("control structure too long");
    }
    SETARG_sBx(jmp, offset);
    state_.bytecode.replaceInstruction(pc, jmp);
}

PatchList CodeGenerator::collectPatchList(i32 list) {
    PatchList result;

    while (list != NO_JUMP) {
        result.append(list);
        list = getjump(list);
    }

    return result;
}


PatchList CodeGenerator::emitComparisonJump(const BinaryExpr& e, bool jumpOnTrue) {
    OpCode op = OpCode::EQ;
    i32 cond = jumpOnTrue ? 1 : 0;
    bool swapOperands = false;

    switch (e.op) {
        case BinaryExpr::Op::Eq:
            op = OpCode::EQ;
            cond = jumpOnTrue ? 1 : 0;
            break;
        case BinaryExpr::Op::Ne:
            op = OpCode::EQ;
            cond = jumpOnTrue ? 0 : 1;
            break;
        case BinaryExpr::Op::Lt:
            op = OpCode::LT;
            cond = jumpOnTrue ? 1 : 0;
            break;
        case BinaryExpr::Op::Le:
            op = OpCode::LE;
            cond = jumpOnTrue ? 1 : 0;
            break;
        case BinaryExpr::Op::Gt:
            op = OpCode::LT;
            cond = jumpOnTrue ? 1 : 0;
            swapOperands = true;
            break;
        case BinaryExpr::Op::Ge:
            op = OpCode::LE;
            cond = jumpOnTrue ? 1 : 0;
            swapOperands = true;
            break;
        default:
            throw std::runtime_error("emitComparisonJump requires comparison operator");
    }

    ValueResult left = emitValue(*e.left);
    ValueResult right = emitValue(*e.right);

    if (swapOperands) {
        std::swap(left, right);
    }

    i32 o1 = valueToRK(left);
    i32 o2 = valueToRK(right);
    freeReg(o1);
    freeReg(o2);

    codeABC(op, cond, o1, o2);

    PatchList result;
    result.append(jump());
    return result;
}

void CodeGenerator::materializeCondResult(const CondResult& cond, i32 reg, bool fallthroughOnTrue) {
    if (fallthroughOnTrue) {
        codeABC(OpCode::LOADBOOL, reg, 1, 1);
        i32 falseLabel = getLabel();
        patchList(cond.falseList, falseLabel);
        codeABC(OpCode::LOADBOOL, reg, 0, 0);
        return;
    }

    codeABC(OpCode::LOADBOOL, reg, 0, 1);
    i32 trueLabel = getLabel();
    patchList(cond.trueList, trueLabel);
    codeABC(OpCode::LOADBOOL, reg, 1, 0);
}

}  // namespace Lua
