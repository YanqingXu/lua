/**
 * @file scope_manager.cpp
 * @brief 作用域管理器实现
 */

#include "compiler/codegen/scope_manager.hpp"
#include "compiler/codegen/codegen.hpp"

#include <stdexcept>

namespace Lua {

ScopeManager::ScopeManager(CodegenState& state, JumpPatcher& jumps) noexcept
    : state_(state)
    , jumps_(jumps) {}

i32 ScopeManager::addLocalVar(const Str& name) {
    i32 reg = state_.registers.current();
    state_.localScope.localVars_.emplace_back(name, reg, state_.bytecode.instructionCount());
    state_.registers.reserve(1);
    state_.registers.checkStack(0);
    return reg;
}

i32 ScopeManager::findLocalVar(const Str& name) const {
    return state_.localScope.findLocal(name);
}

void ScopeManager::markLocalCaptured(i32 reg) {
    state_.localScope.markCaptured(reg);
}

void ScopeManager::adjustLocalVars(i32 count) {
    state_.localScope.activeVarCount_ += count;
    state_.registers.resetToLocals(state_.localScope.activeVarCount_);
    state_.registers.checkStack(0);
}

void ScopeManager::removeLocalVars(i32 toLevel) {
    closeScopeUpvalues(toLevel);
    i32 pc = state_.bytecode.instructionCount();
    state_.localScope.closeLocals(toLevel, pc);
    state_.registers.resetToLocals(state_.localScope.activeVarCount_);
    state_.registers.checkStack(0);
}

void ScopeManager::closeScopeUpvalues(i32 level) {
    if (state_.localScope.activeVarCount_ <= level) {
        return;
    }

    if (!state_.localScope.hasCapturedLocalsFrom(level)) {
        return;
    }

    if (state_.bytecode.hasInstructions() && state_.bytecode.lastOpcode() == OpCode::RETURN) {
        return;
    }

    emitClose(level);
}

i32 ScopeManager::activeLocalCount() const noexcept {
    return state_.localScope.activeVarCount_;
}

const Vec<LocalVar>& ScopeManager::localVars() const noexcept {
    return state_.localScope.localVars_;
}

i32 ScopeManager::findUpvalue(const Str& name) const {
    return state_.upvalueContext.find(name);
}

i32 ScopeManager::addUpvalue(const Str& name, bool inStack, i32 index) {
    return state_.upvalueContext.add(name, inStack, index);
}

i32 ScopeManager::resolveUpvalue(const Str& name) {
    if (state_.parent == nullptr) {
        return -1;
    }

    i32 local = state_.parent->scopes_.findLocalVar(name);
    if (local >= 0) {
        state_.parent->scopes_.markLocalCaptured(local);
        return addUpvalue(name, true, local);
    }

    i32 parentUp = state_.parent->scopes_.resolveUpvalue(name);
    if (parentUp >= 0) {
        return addUpvalue(name, false, parentUp);
    }

    return -1;
}

const Vec<UpvalueCapture>& ScopeManager::upvalues() const noexcept {
    return state_.upvalueContext.upvalues_;
}

void ScopeManager::enterBlock(bool isBreakable) {
    state_.blockManager.enterBlock(isBreakable, state_.localScope.activeVarCount_);
}

void ScopeManager::leaveBlock() {
    if (state_.blockManager.currentBlock_ == nullptr) {
        throw std::runtime_error("No block to leave");
    }

    UPtr<BlockInfo> block = state_.blockManager.takeCurrentBlock();

    removeLocalVars(block->activeVarCount);
    jumps_.patchList(block->breaklist, jumps_.getLabel());
}

BlockInfo* ScopeManager::currentBlock() const noexcept {
    return state_.blockManager.currentBlock_;
}

BlockInfo* ScopeManager::findBreakableBlock() const noexcept {
    BlockInfo* block = state_.blockManager.currentBlock_;
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
