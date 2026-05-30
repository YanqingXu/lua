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

    [[nodiscard]] i32 codeRaw(Instruction inst) {
        jumps_.flushPendingJumps();
        return state_.bytecode.emitRaw(state_.currentLine, inst);
    }

    [[nodiscard]] i32 allocReg() {
        return state_.registers.alloc();
    }

    [[nodiscard]] i32 currentReg() const noexcept {
        return state_.registers.current();
    }

    void freeReg(i32 reg, i32 activeLocalCount) {
        state_.registers.freeReg(reg, activeLocalCount);
    }

    void checkStack(i32 n) {
        state_.registers.checkStack(n);
    }

    void setFreeReg(i32 reg) noexcept {
        state_.registers.setFreeReg(reg);
    }

    void setFreeRegAndCheck(i32 reg) {
        setFreeReg(reg);
        checkStack(0);
    }

    void reserveRegs(i32 count) noexcept {
        state_.registers.reserve(count);
    }

    void reserveRegsAndCheck(i32 count) {
        reserveRegs(count);
        checkStack(0);
    }

    void ensureRegAtLeast(i32 reg) noexcept {
        state_.registers.ensureAtLeast(reg);
    }

    void resetToLocals(i32 activeLocalCount) noexcept {
        state_.registers.resetToLocals(activeLocalCount);
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

    ~LineGuard() noexcept {
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

    ~RegisterGuard() noexcept {
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

class RegisterFrame {
public:
    RegisterFrame(CodegenOps& ops, i32 base) noexcept
        : ops_(ops)
        , base_(base) {
        ops_.setFreeReg(base_);
    }

    [[nodiscard]] i32 base() const noexcept {
        return base_;
    }

    [[nodiscard]] i32 at(i32 offset) const noexcept {
        return base_ + offset;
    }

    void setTop(i32 offset) {
        ops_.setFreeRegAndCheck(at(offset));
    }

    void setTopUnchecked(i32 offset) noexcept {
        ops_.setFreeReg(at(offset));
    }

private:
    CodegenOps& ops_;
    i32 base_;
};

}  // namespace Lua
