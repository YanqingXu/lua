/**
 * @file upvalue.cpp
 * @brief Upvalue类的实现（✅ 改进版 - 使用索引避免悬空指针）
 */

#include "core/upvalue.hpp"
#include "vm/stack.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include <stdexcept>

namespace Lua {

// ========== 静态工厂方法 ==========

Upvalue* Upvalue::createOpen(usize stackIndex) {
    return new Upvalue(stackIndex);
}

Upvalue* Upvalue::createClosed(const Value& value) {
    return new Upvalue(value);
}

// ========== 构造函数 ==========

Upvalue::Upvalue(usize stackIndex)
    : GCObject(GCObjectType::Upval)
    , isOpen_(true)
    , stackIndex_(stackIndex)
    , closedValue_()  // 默认构造为nil
    , next_(nullptr)
{
    // Open状态：只存储索引
}

Upvalue::Upvalue(const Value& value)
    : GCObject(GCObjectType::Upval)
    , isOpen_(false)
    , stackIndex_(0)  // Closed状态下无意义
    , closedValue_(value)
    , next_(nullptr)
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

Value& Upvalue::getValue(Stack& stack) noexcept {
    if (isOpen_) {
        // Open状态：通过索引动态获取栈上的值
        return stack[stackIndex_];
    } else {
        // Closed状态：返回内部存储的值
        return closedValue_;
    }
}

const Value& Upvalue::getValue(const Stack& stack) const noexcept {
    if (isOpen_) {
        // Open状态：通过索引动态获取栈上的值
        return stack[stackIndex_];
    } else {
        // Closed状态：返回内部存储的值
        return closedValue_;
    }
}

void Upvalue::setValue(Stack& stack, const Value& value) {
    if (isOpen_) {
        // Open状态：设置栈上的值
        stack[stackIndex_] = value;
    } else {
        // Closed状态：设置内部存储的值
        closedValue_ = value;
    }
}

// ========== 状态转换 ==========

void Upvalue::close(Stack& stack) {
    if (isClosed()) {
        // 已经是Closed状态，无需操作
        return;
    }

    // 1. 将栈上的值复制到closedValue_
    closedValue_ = stack[stackIndex_];

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

void Upvalue::mark() {
    if (isMarked()) {
        return;  // 已标记，避免重复标记
    }
    
    // 标记自身
    setColor(GCColor::Gray);
    
    // 标记closedValue_中的GC对象
    // 注意：Open状态下，栈上的值由栈管理，不需要在这里标记
    if (isClosed()) {
        if (closedValue_.isString()) {
            GCString* str = closedValue_.asString();
            if (str != nullptr && !str->isMarked()) {
                str->mark();
            }
        } else if (closedValue_.isTable()) {
            Table* table = closedValue_.asTable();
            if (table != nullptr && !table->isMarked()) {
                table->mark();
            }
        } else if (closedValue_.isFunction()) {
            Function* func = closedValue_.asFunction();
            if (func != nullptr && !func->isMarked()) {
                func->mark();
            }
        }
        // 其他GC对象类型（Userdata、Thread）暂未实现
    }
    
    // 标记完成
    setColor(GCColor::Black);
}

usize Upvalue::getSize() const {
    return sizeof(Upvalue);
}

} // namespace Lua

