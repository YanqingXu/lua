/**
 * @file upvalue.cpp
 * @brief Upvalue类的实现（✅ 改进版 - 使用索引避免悬空指针）
 */

#include "core/upvalue.hpp"
#include "vm/state/stack.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "core/userdata.hpp"
#include "gc/garbage_collector.hpp"
#include <stdexcept>

namespace Lua {

// ========== 静态工厂方法 ==========

Upvalue* Upvalue::createOpen(usize stackIndex, Stack& ownerStack) {
    return new Upvalue(stackIndex, ownerStack);
}

Upvalue* Upvalue::createClosed(const Value& value) {
    return new Upvalue(value);
}

// ========== 构造函数 ==========

Upvalue::Upvalue(usize stackIndex, Stack& ownerStack)
    : GCObject(GCObjectType::Upval)
    , isOpen_(true)
    , stackIndex_(stackIndex)
    , closedValue_()  // 默认构造为nil
    , next_(nullptr)
    , ownerStack_(&ownerStack)
{
    // Open状态：存储索引和所属栈指针
}

Upvalue::Upvalue(const Value& value)
    : GCObject(GCObjectType::Upval)
    , isOpen_(false)
    , stackIndex_(0)  // Closed状态下无意义
    , closedValue_(value)
    , next_(nullptr)
    , ownerStack_(nullptr)
{
    // Closed状态：存储值
}

// ========== 状态查询 ==========

bool Upvalue::isOpen() const noexcept {
    return isOpen_;
}

bool Upvalue::isClosed() const noexcept {
    return !isOpen_;
}

// ========== 值访问（✅ 改进版 - 动态计算地址） ==========

Value& Upvalue::getValue(Stack&) noexcept {
    if (isOpen_) {
        // Open状态：使用ownerStack_访问正确的栈（跨协程安全）
        return (*ownerStack_)[stackIndex_];
    } else {
        // Closed状态：返回内部存储的值
        return closedValue_;
    }
}

const Value& Upvalue::getValue(const Stack&) const noexcept {
    if (isOpen_) {
        // Open状态：使用ownerStack_访问正确的栈（跨协程安全）
        return (*ownerStack_)[stackIndex_];
    } else {
        // Closed状态：返回内部存储的值
        return closedValue_;
    }
}

void Upvalue::setValue(Stack&, const Value& value) {
    if (isOpen_) {
        // Open状态：使用ownerStack_设置正确的栈上的值
        (*ownerStack_)[stackIndex_] = value;
    } else {
        // Closed状态：设置内部存储的值
        closedValue_ = value;
    }
}

// ========== 状态转换 ==========

void Upvalue::close(Stack&) {
    if (isClosed()) {
        // 已经是Closed状态，无需操作
        return;
    }

    // 1. 将栈上的值复制到closedValue_（使用ownerStack_确保正确性）
    closedValue_ = (*ownerStack_)[stackIndex_];

    // 2. 标记为Closed状态
    isOpen_ = false;

    // 3. stackIndex_保持不变（用于调试）
}

// ========== 栈索引管理 ==========

usize Upvalue::getStackIndex() const noexcept {
    return stackIndex_;
}

// ========== 链表管理 ==========

Upvalue* Upvalue::getNext() const noexcept {
    return next_;
}

void Upvalue::setNext(Upvalue* next) noexcept {
    next_ = next;
}

// ========== GC支持 ==========

void Upvalue::mark(GarbageCollector& gc) {
    if (isClosed()) {
        gc.markValue(closedValue_);
    } else if (ownerStack_ != nullptr && stackIndex_ < ownerStack_->size()) {
        gc.markValue(ownerStack_->at(stackIndex_));
    }
}

usize Upvalue::getSize() const {
    return sizeof(Upvalue);
}

} // namespace Lua

