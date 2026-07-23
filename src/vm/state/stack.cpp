/**
 * @file stack.cpp
 * @brief Lua栈管理实现
 *
 * @author Lua C++ 项目
 * @date 2025-11-12
 */

#include "vm/state/stack.hpp"
#include "common/lua_error.hpp"
#include <algorithm>
#include <stdexcept>

namespace Lua {

// =====================================================================
// 构造函数
// =====================================================================

Stack::Stack(usize initialSize, LuaAllocator* allocator, const ResourcePolicy* resourcePolicy)
    : stack_(LuaStdAllocator<Value>(allocator)), top_(0), resourcePolicy_(resourcePolicy) {
    const usize requested = std::max(initialSize, MIN_STACK_SIZE);
    const usize limit =
        resourcePolicy_ != nullptr ? std::min(resourcePolicy_->maxStackSlots, MAX_STACK_SIZE) : MAX_STACK_SIZE;
    stack_.resize(std::min(requested, limit));
}

// =====================================================================
// 栈操作（添加预检查和快速push）
// =====================================================================

void Stack::checkSpace(usize needed) {
    const usize limit =
        resourcePolicy_ != nullptr ? std::min(resourcePolicy_->maxStackSlots, MAX_STACK_SIZE) : MAX_STACK_SIZE;
    if (top_ > limit || needed > limit - top_) {
        throw StackOverflowError("stack overflow: resource stack slot limit exceeded");
    }

    usize available = stack_.size() - top_;

    if (available < needed) {
        ensureSpace(needed);
    }
}

void Stack::checkLimit(usize newTop) const {
    const usize limit =
        resourcePolicy_ != nullptr ? std::min(resourcePolicy_->maxStackSlots, MAX_STACK_SIZE) : MAX_STACK_SIZE;
    if (newTop > limit) {
        throw StackOverflowError("stack overflow: resource stack slot limit exceeded");
    }
}

// =====================================================================
// 快速push，不检查边界。调用前必须确保有足够空间（使用checkSpace）
// =====================================================================

void Stack::pushUnchecked(const Value& value) noexcept {
    stack_[top_++] = value;
}

void Stack::push(const Value& value) {
    checkSpace(1);
    pushUnchecked(value);
}

Value Stack::pop() {
    if (empty()) {
        throw RuntimeError("Stack underflow: cannot pop from empty stack");
    }

    return stack_[--top_];
}

Value& Stack::top() {
    if (empty()) {
        throw RuntimeError("Stack is empty: cannot access top");
    }

    return stack_[top_ - 1];
}

const Value& Stack::top() const {
    if (empty()) {
        throw RuntimeError("Stack is empty: cannot access top");
    }

    return stack_[top_ - 1];
}

Value& Stack::at(usize index) {
    if (index >= top_) {
        throw std::out_of_range("Stack index out of range");
    }

    return stack_[index];
}

const Value& Stack::at(usize index) const {
    if (index >= top_) {
        throw std::out_of_range("Stack index out of range");
    }

    return stack_[index];
}

// =====================================================================
// 栈空间管理（添加最大栈限制检查）
// =====================================================================

void Stack::ensureSpace(usize needed) {
    const usize limit =
        resourcePolicy_ != nullptr ? std::min(resourcePolicy_->maxStackSlots, MAX_STACK_SIZE) : MAX_STACK_SIZE;
    if (top_ > limit || needed > limit - top_) {
        throw StackOverflowError("stack overflow: resource stack slot limit exceeded");
    }
    usize available = stack_.size() - top_;

    if (available < needed) {
        const usize required = top_ + needed;
        const usize doubled = stack_.size() > limit / 2 ? limit : stack_.size() * 2;
        const usize padded = required > limit - std::min(limit, EXTRA_STACK) ? limit : required + EXTRA_STACK;
        const usize newCapacity = std::max(doubled, padded);

        stack_.resize(newCapacity);
    }
}

void Stack::setTop(usize newTop) {
    checkLimit(newTop);
    if (newTop > stack_.size()) {
        const usize limit =
            resourcePolicy_ != nullptr ? std::min(resourcePolicy_->maxStackSlots, MAX_STACK_SIZE) : MAX_STACK_SIZE;
        const usize newCapacity = newTop > limit - std::min(limit, EXTRA_STACK) ? limit : newTop + EXTRA_STACK;

        // 需要扩展栈
        stack_.resize(newCapacity);
    }

    // 如果新栈顶大于当前栈顶，用nil填充
    if (newTop > top_) {
        for (usize i = top_; i < newTop; ++i) {
            stack_[i] = Value(); // nil值
        }
    }

    top_ = newTop;
}

} // namespace Lua
