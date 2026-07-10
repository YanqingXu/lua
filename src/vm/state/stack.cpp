/**
 * @file stack.cpp
 * @brief Lua栈管理实现
 * 
 * @author Lua C++ Project
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

Stack::Stack(usize initialSize, LuaAllocator* allocator)
    : stack_(LuaStdAllocator<Value>(allocator))
    , top_(0)
{
    stack_.resize(initialSize);
    // 确保初始大小至少为MIN_STACK_SIZE
    if (initialSize < MIN_STACK_SIZE) {
        stack_.resize(MIN_STACK_SIZE);
    }
}

// =====================================================================
// 栈操作（添加预检查和快速push）
// =====================================================================

void Stack::checkSpace(usize needed) {
    usize available = stack_.size() - top_;

    if (available < needed) {
        ensureSpace(needed);
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
    usize available = stack_.size() - top_;

    if (available < needed) {
        usize newCapacity = std::max(
            stack_.size() * 2,
            top_ + needed + EXTRA_STACK
        );

        // 检查是否超过最大栈限制
        if (newCapacity > MAX_STACK_SIZE) {
            // 如果确实需要超过限制，抛出异常
            if (top_ + needed > MAX_STACK_SIZE) {
                throw StackOverflowError("stack overflow: maximum stack size exceeded");
            }
            // 否则，限制在最大值
            newCapacity = MAX_STACK_SIZE;
        }

        stack_.resize(newCapacity);
    }
}

void Stack::setTop(usize newTop) {
    if (newTop > stack_.size()) {
        usize newCapacity = newTop + EXTRA_STACK;
        if (newCapacity > MAX_STACK_SIZE) {
            if (newTop > MAX_STACK_SIZE) {
                throw StackOverflowError("stack overflow: maximum stack size exceeded");
            }
            newCapacity = MAX_STACK_SIZE;
        }

        // 需要扩展栈
        stack_.resize(newCapacity);
    }

    // 如果新栈顶大于当前栈顶，用nil填充
    if (newTop > top_) {
        for (usize i = top_; i < newTop; ++i) {
            stack_[i] = Value();  // nil值
        }
    }

    top_ = newTop;
}

} // namespace Lua

