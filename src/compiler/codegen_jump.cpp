/**
 * @file codegen_jump.cpp
 * @brief CodeGenerator jump list and condition materialization helpers.
 */

#include "compiler/codegen.hpp"

#include <stdexcept>
#include <utility>

namespace Lua {

i32 CodeGenerator::jump() {
    i32 jpc = state_.blocks.jpc_;  // 保存当前待处理跳转列表
    state_.blocks.jpc_ = NO_JUMP;  // 清空state_.blocks.jpc_
    i32 j = codeAsBx(OpCode::JMP, 0, NO_JUMP);  // 生成JMP指令
    concatJumpList(j, jpc);  // 将jpc链表连接到j后面
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
    // 将所有待处理跳转修补到当前指令位置
    i32 target = static_cast<i32>(state_.proto->getInstructionCount());
    patchList(state_.blocks.jpc_, target);
    state_.blocks.jpc_ = NO_JUMP;
}

i32 CodeGenerator::getLabel() {
    return static_cast<i32>(state_.proto->getInstructionCount());
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

// invertJump / jumponcond are no longer used

i32 CodeGenerator::condjump(OpCode op, i32 a, i32 b, i32 c) {
    // 当 TESTSET 的 A 参数为 NO_REG 时转换为 TEST：
    // NO_REG(255) 是无效寄存器索引，直接使用会导致 VM 运行时错误
    if (op == OpCode::TESTSET && a == NO_REG) {
        // 转换为TEST指令：TEST A B C
        // TESTSET的B参数变为TEST的A参数（被测试的寄存器）
        op = OpCode::TEST;
        a = b;
        b = 0;
    }

    codeABC(op, a, b, c);
    i32 jpc = state_.blocks.jpc_;
    state_.blocks.jpc_ = NO_JUMP;
    i32 j = codeAsBx(OpCode::JMP, 0, NO_JUMP);
    concatJumpList(j, jpc);  // ⭐ 将jpc链表连接到j后面
    return j;
}

void CodeGenerator::patchtohere(i32 list) {
    state_.pc = static_cast<i32>(state_.proto->getInstructionCount());
    concatJumpList(state_.blocks.jpc_, list);
}

void CodeGenerator::patchtohere(const PatchList& list) {
    state_.pc = static_cast<i32>(state_.proto->getInstructionCount());
    patchList(list, state_.pc);
}

void CodeGenerator::syncPC() {
    state_.pc = static_cast<i32>(state_.proto->getInstructionCount());
}

i32 CodeGenerator::getjump(i32 pc) {
    Instruction inst = state_.proto->getInstruction(pc);
    i32 offset = GETARG_sBx(inst);
    if (offset == NO_JUMP) {
        return NO_JUMP;
    } else {
        return (pc + 1) + offset;
    }
}

void CodeGenerator::fixjump(i32 pc, i32 dest) {
    Instruction jmp = state_.proto->getInstruction(pc);
    i32 offset = dest - (pc + 1);
    if (offset > MAXARG_sBx || offset < -MAXARG_sBx) {
        throw std::runtime_error("control structure too long");
    }
    SETARG_sBx(jmp, offset);
    state_.proto->setInstruction(pc, jmp);
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

    // PR-6: native ValueResult pipeline
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