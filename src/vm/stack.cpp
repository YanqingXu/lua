/**
 * @file stack.cpp
 * @brief Lua栈管理实现
 * 
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "vm/stack.hpp"
#include <stdexcept>
#include <algorithm>

namespace Lua {

// =====================================================================
// 构造函数
// =====================================================================

Stack::Stack(usize initialSize)
    : stack_(initialSize)
    , top_(0)
{
    // 确保初始大小至少为MIN_STACK_SIZE
    if (initialSize < MIN_STACK_SIZE) {
        stack_.resize(MIN_STACK_SIZE);
    }
}

// =====================================================================
// 栈操作
// =====================================================================

void Stack::push(const Value& value) {
    // 确保有足够空间
    ensureSpace(1);
    
    // 压入值
    stack_[top_++] = value;
}

Value Stack::pop() {
    if (empty()) {
        throw std::runtime_error("Stack underflow: cannot pop from empty stack");
    }
    
    return stack_[--top_];
}

Value& Stack::top() {
    if (empty()) {
        throw std::runtime_error("Stack is empty: cannot access top");
    }
    
    return stack_[top_ - 1];
}

const Value& Stack::top() const {
    if (empty()) {
        throw std::runtime_error("Stack is empty: cannot access top");
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
// 栈空间管理
// =====================================================================

void Stack::ensureSpace(usize needed) {
    usize available = stack_.size() - top_;
    
    if (available < needed) {
        // 需要扩展栈
        // 新容量 = max(当前容量 * 2, 当前大小 + 需要的空间 + EXTRA_STACK)
        usize newCapacity = std::max(
            stack_.size() * 2,
            top_ + needed + EXTRA_STACK
        );
        
        stack_.resize(newCapacity);
    }
}

void Stack::setTop(usize newTop) {
    if (newTop > stack_.size()) {
        // 需要扩展栈
        stack_.resize(newTop + EXTRA_STACK);
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

