/**
 * @file upvalue.cpp
 * @brief Upvalue类的实现
 */

#include "core/upvalue.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include <stdexcept>

namespace Lua {

// ========== 静态工厂方法 ==========

Upvalue* Upvalue::createOpen(Value* stackValue, usize stackIndex) {
    if (stackValue == nullptr) {
        throw std::invalid_argument("Upvalue::createOpen: stackValue cannot be null");
    }
    return new Upvalue(stackValue, stackIndex);
}

Upvalue* Upvalue::createClosed(const Value& value) {
    return new Upvalue(value);
}

// ========== 构造函数 ==========

Upvalue::Upvalue(Value* stackValue, usize stackIndex)
    : GCObject(GCObjectType::Upval)
    , v_(stackValue)
    , stackIndex_(stackIndex)
    , closedValue_()  // 默认构造为nil
    , next_(nullptr)
{
    // Open状态：v_指向栈上的Value
}

Upvalue::Upvalue(const Value& value)
    : GCObject(GCObjectType::Upval)
    , v_(&closedValue_)
    , stackIndex_(0)  // Closed状态下无意义
    , closedValue_(value)
    , next_(nullptr)
{
    // Closed状态：v_指向closedValue_
}

// ========== 状态查询 ==========

bool Upvalue::isOpen() const noexcept {
    // Open状态：v_不指向closedValue_
    return v_ != &closedValue_;
}

bool Upvalue::isClosed() const noexcept {
    // Closed状态：v_指向closedValue_
    return v_ == &closedValue_;
}

// ========== 值访问 ==========

Value& Upvalue::getValue() noexcept {
    return *v_;
}

const Value& Upvalue::getValue() const noexcept {
    return *v_;
}

// ========== 状态转换 ==========

void Upvalue::close() {
    if (isClosed()) {
        // 已经是Closed状态，无需操作
        return;
    }
    
    // 1. 将栈上的值复制到closedValue_
    closedValue_ = *v_;
    
    // 2. 更新v_指针指向closedValue_
    v_ = &closedValue_;
    
    // 3. stackIndex_变为无效（保持原值，但不再使用）
    // 注意：不清零stackIndex_，因为可能在调试时有用
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

