/**
 * @file scope_manager.cpp
 * @brief ScopeManager implementation.
 */

#include "compiler/codegen/scope_manager.hpp"
#include "compiler/codegen/codegen.hpp"

#include <stdexcept>

namespace Lua {

ScopeManager::ScopeManager(CodegenState& state, JumpPatcher& jumps) noexcept
    : state_(state)
    , jumps_(jumps) {}

i32 ScopeManager::addLocalVar(const Str& name) {
    i32 reg = state_.regs.current();
    state_.locals.localVars_.emplace_back(name, reg, state_.bytecode.instructionCount());
    state_.regs.reserve(1);
    state_.regs.checkStack(0);
    return reg;
}

i32 ScopeManager::findLocalVar(const Str& name) const {
    return state_.locals.findLocal(name);
}

void ScopeManager::adjustLocalVars(i32 count) {
    state_.locals.nactvar_ += count;
    state_.regs.resetToLocals(state_.locals.nactvar_);
    state_.regs.checkStack(0);
}

void ScopeManager::removeLocalVars(i32 toLevel) {
    closeScopeUpvalues(toLevel);
    i32 pc = state_.bytecode.instructionCount();
    state_.locals.closeLocals(toLevel, pc);
    state_.regs.resetToLocals(state_.locals.nactvar_);
    state_.regs.checkStack(0);
}

void ScopeManager::closeScopeUpvalues(i32 level) {
    if (state_.locals.nactvar_ <= level) {
        return;
    }

    if (state_.bytecode.hasInstructions() && state_.bytecode.lastOpcode() == OpCode::RETURN) {
        return;
    }

    emitClose(level);
}

i32 ScopeManager::activeLocalCount() const noexcept {
    return state_.locals.nactvar_;
}

const Vec<LocalVar>& ScopeManager::localVars() const noexcept {
    return state_.locals.localVars_;
}

i32 ScopeManager::findUpvalue(const Str& name) const {
    return state_.upvalues.find(name);
}

i32 ScopeManager::addUpvalue(const Str& name, bool inStack, i32 index) {
    return state_.upvalues.add(name, inStack, index);
}

i32 ScopeManager::resolveUpvalue(const Str& name) {
    if (state_.parent == nullptr) {
        return -1;
    }

    i32 local = state_.parent->findLocalVar(name);
    if (local >= 0) {
        return addUpvalue(name, true, local);
    }

    i32 parentUp = state_.parent->resolveUpvalue(name);
    if (parentUp >= 0) {
        return addUpvalue(name, false, parentUp);
    }

    return -1;
}

const Vec<UpvalueCapture>& ScopeManager::upvalues() const noexcept {
    return state_.upvalues.upvalues_;
}

void ScopeManager::enterBlock(bool isBreakable) {
    state_.blocks.enterBlock(isBreakable, state_.locals.nactvar_);
}

void ScopeManager::leaveBlock() {
    if (state_.blocks.currentBlock_ == nullptr) {
        throw std::runtime_error("No block to leave");
    }

    BlockInfo* block = state_.blocks.currentBlock_;
    state_.blocks.currentBlock_ = block->previous;

    removeLocalVars(block->nactvar);
    jumps_.patchToHere(block->breaklist);

    delete block;
}

BlockInfo* ScopeManager::currentBlock() const noexcept {
    return state_.blocks.currentBlock_;
}

BlockInfo* ScopeManager::findBreakableBlock() const noexcept {
    BlockInfo* block = state_.blocks.currentBlock_;
    while (block != nullptr && !block->isbreakable) {
        block = block->previous;
    }
    return block;
}

void ScopeManager::appendBreakJump(BlockInfo& block, i32 jumpPc) {
    jumps_.concatJumpList(block.breaklist, jumpPc);
}

void ScopeManager::emitClose(i32 level) {
    jumps_.flushPendingJumps();
    state_.bytecode.emitABC(state_.currentLine, OpCode::CLOSE, level, 0, 0);
}

}  // namespace Lua
