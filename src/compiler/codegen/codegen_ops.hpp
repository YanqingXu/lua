#pragma once

/**
 * @file codegen_ops.hpp
 * @brief Shared low-level operations for codegen implementation slices.
 */

#include "compiler/codegen/codegen_state.hpp"
#include "compiler/codegen/jump_patcher.hpp"
#include "compiler/opcode.hpp"

namespace Lua {

/**
 * Small facade for repeated low-level mutations during lowering.
 *
 * Emitters still own semantic decisions. CodegenOps centralizes mechanical
 * details: pending jump flushes before instruction emission, instruction
 * argument rewrites, and common register/constant utilities.
 */
class CodegenOps {
public:
    CodegenOps(CodegenState& state, JumpPatcher& jumps) noexcept
        : state_(state)
        , jumps_(jumps) {}

    [[nodiscard]] i32 codeABC(OpCode op, i32 a, i32 b, i32 c) {
        jumps_.flushPendingJumps();
        return state_.bytecode.emitABC(state_.currentLine, op, a, b, c);
    }

    [[nodiscard]] i32 codeABx(OpCode op, i32 a, i32 bx) {
        jumps_.flushPendingJumps();
        return state_.bytecode.emitABx(state_.currentLine, op, a, bx);
    }

    [[nodiscard]] i32 codeAsBx(OpCode op, i32 a, i32 sbx) {
        jumps_.flushPendingJumps();
        return state_.bytecode.emitAsBx(state_.currentLine, op, a, sbx);
    }

    [[nodiscard]] i32 allocReg() {
        return state_.registers.alloc();
    }

    void freeReg(i32 reg, i32 activeLocalCount) {
        state_.registers.freeReg(reg, activeLocalCount);
    }

    void checkStack(i32 n) {
        state_.registers.checkStack(n);
    }

    [[nodiscard]] i32 numberConstant(f64 value) {
        return state_.bytecode.addNumberConstant(value);
    }

    [[nodiscard]] i32 stringConstant(const Str& value) {
        return state_.bytecode.addStringConstant(value);
    }

    [[nodiscard]] Instruction instruction(i32 pc) const {
        return state_.bytecode.instruction(pc);
    }

    void replaceInstruction(i32 pc, Instruction inst) {
        state_.bytecode.replaceInstruction(pc, inst);
    }

    void patchArgA(i32 pc, i32 a) {
        Instruction inst = instruction(pc);
        SETARG_A(inst, a);
        replaceInstruction(pc, inst);
    }

    void patchArgB(i32 pc, i32 b) {
        Instruction inst = instruction(pc);
        SETARG_B(inst, b);
        replaceInstruction(pc, inst);
    }

    void patchArgC(i32 pc, i32 c) {
        Instruction inst = instruction(pc);
        SETARG_C(inst, c);
        replaceInstruction(pc, inst);
    }

    void patchArgsAB(i32 pc, i32 a, i32 b) {
        Instruction inst = instruction(pc);
        SETARG_A(inst, a);
        SETARG_B(inst, b);
        replaceInstruction(pc, inst);
    }

    void patchArgsBC(i32 pc, i32 b, i32 c) {
        Instruction inst = instruction(pc);
        SETARG_B(inst, b);
        SETARG_C(inst, c);
        replaceInstruction(pc, inst);
    }

    void patchToABC(i32 pc, OpCode op, i32 a, i32 b, i32 c) {
        replaceInstruction(pc, CREATE_ABC(op, a, b, c));
    }

private:
    CodegenState& state_;
    JumpPatcher& jumps_;
};

class LineGuard {
public:
    LineGuard(CodegenState& state, i32 line) noexcept
        : state_(state)
        , previousLine_(state.currentLine) {
        if (line > 0) {
            state_.currentLine = line;
        }
    }

    LineGuard(const LineGuard&) = delete;
    LineGuard& operator=(const LineGuard&) = delete;

    ~LineGuard() {
        state_.currentLine = previousLine_;
    }

private:
    CodegenState& state_;
    i32 previousLine_;
};

class RegisterGuard {
public:
    explicit RegisterGuard(CodegenState& state) noexcept
        : state_(state)
        , savedFreeReg_(state.registers.current()) {}

    RegisterGuard(const RegisterGuard&) = delete;
    RegisterGuard& operator=(const RegisterGuard&) = delete;

    ~RegisterGuard() {
        if (active_) {
            state_.registers.restore(savedFreeReg_);
        }
    }

    [[nodiscard]] i32 savedFreeReg() const noexcept {
        return savedFreeReg_;
    }

    void restoreNow() noexcept {
        if (active_) {
            state_.registers.restore(savedFreeReg_);
            active_ = false;
        }
    }

    void dismiss() noexcept {
        active_ = false;
    }

private:
    CodegenState& state_;
    i32 savedFreeReg_;
    bool active_ = true;
};

}  // namespace Lua
