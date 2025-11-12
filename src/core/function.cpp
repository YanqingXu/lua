/**
 * @file function.cpp
 * @brief 函数对象实现
 */

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include <stdexcept>

namespace Lua {

// =====================================================================
// Proto类实现
// =====================================================================

Proto::Proto()
    : GCObject(GCObjectType::Proto)
    , numParams_(0)
    , isVararg_(false)
    , maxStackSize_(0)
    , source_(nullptr)
    , constants_()
{
}

Proto::~Proto() {
    // 常量表中的GC对象由GC系统管理，这里不需要手动删除
}

usize Proto::addConstant(const Value& value) {
    constants_.push_back(value);
    return constants_.size() - 1;
}

Value Proto::getConstant(usize index) const {
    if (index >= constants_.size()) {
        throw std::out_of_range("Constant index out of range");
    }
    return constants_[index];
}

void Proto::mark() {
    // 标记源文件名
    if (source_ != nullptr) {
        source_->setColor(GCColor::Gray);
    }
    
    // 标记常量表中的GC对象
    for (const Value& val : constants_) {
        if (val.isString()) {
            val.asString()->setColor(GCColor::Gray);
        } else if (val.isTable()) {
            val.asTable()->setColor(GCColor::Gray);
        } else if (val.isFunction()) {
            val.asFunction()->setColor(GCColor::Gray);
        }
        // TODO: 添加Userdata和Thread的标记
    }
}

usize Proto::getSize() const {
    // 基础大小 + 常量表大小
    return sizeof(Proto) + constants_.capacity() * sizeof(Value);
}

// =====================================================================
// Function类实现
// =====================================================================

Function::Function(CFunction func)
    : GCObject(GCObjectType::Function)
    , isC_(true)
    , cFunction_(func)
    , proto_(nullptr)
{
    if (func == nullptr) {
        throw std::invalid_argument("C function pointer cannot be null");
    }
}

Function::Function(Proto* proto)
    : GCObject(GCObjectType::Function)
    , isC_(false)
    , cFunction_(nullptr)
    , proto_(proto)
{
    if (proto == nullptr) {
        throw std::invalid_argument("Proto pointer cannot be null");
    }
}

Function::~Function() {
    // Proto和Upvalue由GC系统管理，这里不需要手动删除
}

// =====================================================================
// Upvalue管理
// =====================================================================

Upvalue* Function::getUpvalue(usize index) const {
    if (index >= upvalues_.size()) {
        return nullptr;
    }
    return upvalues_[index];
}

void Function::setUpvalue(usize index, Upvalue* upvalue) {
    if (index >= upvalues_.size()) {
        throw std::out_of_range("Upvalue index out of range");
    }
    upvalues_[index] = upvalue;
}

void Function::addUpvalue(Upvalue* upvalue) {
    upvalues_.push_back(upvalue);
}

// =====================================================================
// GC支持
// =====================================================================

void Function::mark() {
    // 如果是Lua函数，标记函数原型
    if (!isC_ && proto_ != nullptr) {
        proto_->setColor(GCColor::Gray);
    }

    // 标记所有upvalue
    for (Upvalue* uv : upvalues_) {
        if (uv != nullptr && !uv->isMarked()) {
            uv->mark();
        }
    }
}

usize Function::getSize() const {
    // 基础大小 + upvalue数组大小
    return sizeof(Function) + upvalues_.capacity() * sizeof(Upvalue*);
}

} // namespace Lua

