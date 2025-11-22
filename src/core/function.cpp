/**
 * @file function.cpp
 * @brief 函数对象实现
 */

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include <stdexcept>
#include <iostream>

namespace Lua {

// =====================================================================
// Proto类实现
// =====================================================================

Proto::Proto()
    : GCObject(GCObjectType::Proto)
    , constants_()
    , code_()
    , subProtos_()
    , lineInfo_()
    , locvars_()
    , upvalueNames_()
    , source_(nullptr)
    , linedefined_(0)
    , lastlinedefined_(0)
    , gclist_(nullptr)
    , nups_(0)
    , numParams_(0)
    , isVararg_(0)
    , maxStackSize_(0)
{
}

Proto::~Proto() {
    // 常量表中的GC对象由GC系统管理，这里不需要手动删除
}

usize Proto::addConstant(const Value& value) {
    usize index = constants_.size();
    constants_.push_back(value);
    #ifdef DEBUG
    std::cerr << "[Proto::addConstant] Proto=" << (void*)this
              << " index=" << index
              << " value=" << value.toString() << std::endl;
    #endif
    return index;
}

Value Proto::getConstant(usize index) const {
    if (index >= constants_.size()) {
        throw std::out_of_range("Constant index out of range");
    }
    Value result = constants_[index];
    #ifdef DEBUG
    std::cerr << "[Proto::getConstant] Proto=" << (void*)this
              << " index=" << index
              << " value=" << result.toString()
              << " (total=" << constants_.size() << ")" << std::endl;
    #endif
    return result;
}

usize Proto::addInstruction(Instruction inst) {
    code_.push_back(inst);
    return code_.size() - 1;
}

Instruction Proto::getInstruction(usize index) const {
    if (index >= code_.size()) {
        throw std::out_of_range("Instruction index out of range");
    }
    return code_[index];
}

void Proto::setInstruction(usize index, Instruction inst) {
    if (index >= code_.size()) {
        throw std::out_of_range("Instruction index out of range");
    }
    code_[index] = inst;
}

void Proto::addLineInfo(i32 line) {
    lineInfo_.push_back(line);
}

i32 Proto::getLine(usize pc) const {
    if (pc >= lineInfo_.size()) {
        return 0;  // 未知行号
    }
    return lineInfo_[pc];
}

usize Proto::addProto(Proto* proto) {
    subProtos_.push_back(proto);
    return subProtos_.size() - 1;
}

Proto* Proto::getSubProto(usize index) const {
    if (index >= subProtos_.size()) {
        throw std::out_of_range("Sub-proto index out of range");
    }
    return subProtos_[index];
}

// =====================================================================
// 局部变量信息管理
// =====================================================================

usize Proto::addLocVar(GCString* varname, i32 startpc, i32 endpc) {
    locvars_.emplace_back(varname, startpc, endpc);
    return locvars_.size() - 1;
}

const LocVar& Proto::getLocVar(usize index) const {
    if (index >= locvars_.size()) {
        throw std::out_of_range("LocVar index out of range");
    }
    return locvars_[index];
}

const char* Proto::getLocalName(i32 localNumber, i32 pc) const {
    // 遍历局部变量信息数组
    for (usize i = 0; i < locvars_.size() && locvars_[i].startpc <= pc; i++) {
        // 检查变量是否在指定pc位置活跃
        if (pc < locvars_[i].endpc) {
            localNumber--;
            if (localNumber == 0) {
                return locvars_[i].varname ? locvars_[i].varname->c_str() : nullptr;
            }
        }
    }
    return nullptr;  // 未找到对应的局部变量
}

// =====================================================================
// 上值名称管理
// =====================================================================

usize Proto::addUpvalueName(GCString* name) {
    upvalueNames_.push_back(name);
    return upvalueNames_.size() - 1;
}

GCString* Proto::getUpvalueName(usize index) const {
    if (index >= upvalueNames_.size()) {
        return nullptr;
    }
    return upvalueNames_[index];
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

    // 标记子函数原型
    for (Proto* subProto : subProtos_) {
        if (subProto != nullptr) {
            subProto->setColor(GCColor::Gray);
        }
    }

    // 标记局部变量名称
    for (const LocVar& locvar : locvars_) {
        if (locvar.varname != nullptr) {
            locvar.varname->setColor(GCColor::Gray);
        }
    }

    // 标记上值名称
    for (GCString* name : upvalueNames_) {
        if (name != nullptr) {
            name->setColor(GCColor::Gray);
        }
    }
}

usize Proto::getSize() const {
    // 基础大小 + 所有动态数组的容量
    return sizeof(Proto)
         + constants_.capacity() * sizeof(Value)
         + code_.capacity() * sizeof(Instruction)
         + lineInfo_.capacity() * sizeof(i32)
         + subProtos_.capacity() * sizeof(Proto*)
         + locvars_.capacity() * sizeof(LocVar)
         + upvalueNames_.capacity() * sizeof(GCString*);
}

// =====================================================================
// Function类实现
// =====================================================================

Function::Function(CFunction func)
    : GCObject(GCObjectType::Function)
    , isC_(true)
    , cFunction_(func)
    , proto_(nullptr)
    , env_(nullptr)  // 初始化环境表为nullptr
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
    , env_(nullptr)  // 初始化环境表为nullptr
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

    // 标记环境表（Lua 5.1兼容）
    if (env_ != nullptr) {
        env_->mark();
    }
}

usize Function::getSize() const {
    // 基础大小 + upvalue数组大小
    return sizeof(Function) + upvalues_.capacity() * sizeof(Upvalue*);
}

} // namespace Lua

