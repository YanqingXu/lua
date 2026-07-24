/**
 * @file function.cpp
 * @brief 函数对象实现
 */

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "gc/garbage_collector.hpp"
#include <stdexcept>
#include <iostream>

namespace Lua {

// =====================================================================
// Proto类实现
// =====================================================================

Proto::Proto() : Proto(nullptr) {}

Proto::Proto(LuaAllocator* allocator)
    : GCObject(GCObjectType::Proto), constants_(allocator),
      constantMap_(0, ConstantKeyHash{}, std::equal_to<ConstantKey>{}, ConstantMapAllocator(allocator)),
      code_(allocator), subProtos_(allocator), lineInfo_(allocator), locvars_(allocator), upvalueNames_(allocator),
      source_(nullptr), linedefined_(0), lastlinedefined_(0), gclist_(nullptr), nups_(0), numParams_(0), isVararg_(0),
      maxStackSize_(0) {}

Proto::~Proto() {
    // 常量表中的GC对象由GC系统管理，这里不需要手动删除
}

void Proto::setSource(GCString* src) {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, src);
    }
    source_ = src;
}

usize Proto::addConstant(const Value& value) {
    // =====================================================================
    // 常量去重逻辑（参考Lua 5.1的addk()函数）
    // =====================================================================
    //
    // 对于常量类型（nil/bool/number/string），先在哈希表中查找：
    // - 如果找到相同的常量，直接返回已有索引，避免重复存储
    // - 如果未找到，添加新常量并记录到哈希表中
    //
    // 注意：nil常量的去重需要特殊处理。Lua 5.1原版使用哈希表本身作为
    // nil的键（因为nil不能作为Lua表的键），这里使用std::monostate作为
    // nil的键，配合独立的标志位确保nil只存储一次。
    // =====================================================================

    // 仅对常量类型（nil/bool/number/string）执行去重
    if (value.isNil() || value.isBoolean() || value.isNumber() || value.isString()) {
        ConstantKey key = ConstantKey::fromValue(value);
        auto it = constantMap_.find(key);
        if (it != constantMap_.end()) {
            // 常量已存在，返回已有索引
            return it->second;
        }
        // 常量不存在，添加新条目
        usize index = constants_.size();
        if (GarbageCollector* gc = getOwnerCollector()) {
            gc->writeBarrier(this, value);
        }
        constants_.push_back(value);
        try {
            auto [entry, inserted] = constantMap_.emplace(key, index);
            if (!inserted) {
                constants_.pop_back();
                if (GarbageCollector* gc = getOwnerCollector()) {
                    gc->accountObjectSizeChange(this);
                }
                return entry->second;
            }
        } catch (...) {
            constants_.pop_back();
            if (GarbageCollector* gc = getOwnerCollector()) {
                gc->accountObjectSizeChange(this);
            }
            throw;
        }
        if (GarbageCollector* gc = getOwnerCollector()) {
            gc->accountObjectSizeChange(this);
        }
        return index;
    }

    // 非常量类型（table/function等）不参与去重，直接添加
    usize index = constants_.size();
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, value);
    }
    constants_.push_back(value);
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
    return index;
}

usize Proto::appendConstantSlot(const Value& value) {
    usize index = constants_.size();
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, value);
    }
    constants_.push_back(value);
    try {
        if (value.isNil() || value.isBoolean() || value.isNumber() || value.isString()) {
            ConstantKey key = ConstantKey::fromValue(value);
            constantMap_.emplace(key, index);
        }
    } catch (...) {
        constants_.pop_back();
        if (GarbageCollector* gc = getOwnerCollector()) {
            gc->accountObjectSizeChange(this);
        }
        throw;
    }
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }

    return index;
}

Value Proto::getConstant(usize index) const {
    if (index >= constants_.size()) {
        throw std::out_of_range("Constant index out of range");
    }
    Value result = constants_[index];
    /** @brief 禁用调试输出，以保持字节码打印结果整洁。 */
    // #ifdef DEBUG
    // std::cerr << "[Proto::getConstant] Proto=" << (void*)this
    //           << " index=" << index
    //           << " value=" << result.toString()
    //           << " (total=" << constants_.size() << ")" << std::endl;
    // #endif
    return result;
}

usize Proto::addInstruction(Instruction inst) {
    code_.push_back(inst);
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
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
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
}

i32 Proto::getLine(usize pc) const {
    if (pc >= lineInfo_.size()) {
        return 0; // 未知行号
    }
    return lineInfo_[pc];
}

usize Proto::addProto(Proto* proto) {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, proto);
    }
    subProtos_.push_back(proto);
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
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

usize Proto::addLocVar(GCString* varname, i32 startpc, i32 endpc, i32 reg) {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, varname);
    }
    locvars_.emplace_back(varname, startpc, endpc, reg);
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
    return locvars_.size() - 1;
}

const LocVar& Proto::getLocVar(usize index) const {
    if (index >= locvars_.size()) {
        throw std::out_of_range("LocVar index out of range");
    }
    return locvars_[index];
}

const LocVar* Proto::getLocalVarInfo(i32 localNumber, i32 pc) const {
    // 遍历局部变量信息数组
    for (usize i = 0; i < locvars_.size() && locvars_[i].startpc <= pc; i++) {
        // 检查变量是否在指定pc位置活跃
        if (pc < locvars_[i].endpc) {
            localNumber--;
            if (localNumber == 0) {
                return &locvars_[i];
            }
        }
    }
    return nullptr; // 未找到对应的局部变量
}

CharPtr Proto::getLocalName(i32 localNumber, i32 pc) const {
    const LocVar* locvar = getLocalVarInfo(localNumber, pc);
    if (locvar == nullptr) {
        return nullptr;
    }

    return locvar->varname ? locvar->varname->c_str() : nullptr;
}

// =====================================================================
// 上值名称管理
// =====================================================================

usize Proto::addUpvalueName(GCString* name) {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, name);
    }
    upvalueNames_.push_back(name);
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
    return upvalueNames_.size() - 1;
}

GCString* Proto::getUpvalueName(usize index) const {
    if (index >= upvalueNames_.size()) {
        return nullptr;
    }
    return upvalueNames_[index];
}

void Proto::mark(GarbageCollector& gc) {
    // 标记源文件名
    gc.markObject(source_);

    // 标记常量表中的GC对象
    for (const Value& val : constants_) {
        gc.markValue(val);
    }

    // 标记子函数原型
    for (Proto* subProto : subProtos_) {
        gc.markObject(subProto);
    }

    // 标记局部变量名称
    for (const LocVar& locvar : locvars_) {
        gc.markObject(locvar.varname);
    }

    // 标记上值名称
    for (GCString* name : upvalueNames_) {
        gc.markObject(name);
    }
}

usize Proto::getSize() const {
    // 基础大小 + 所有动态数组的容量
    return sizeof(Proto) + constants_.capacity() * sizeof(Value) + code_.capacity() * sizeof(Instruction) +
           lineInfo_.capacity() * sizeof(i32) + subProtos_.capacity() * sizeof(Proto*) +
           locvars_.capacity() * sizeof(LocVar) + upvalueNames_.capacity() * sizeof(GCString*);
}

// =====================================================================
// Function类实现
// =====================================================================

Function::Function(CFunction func) : Function(nullptr, func) {}

Function::Function(LuaAllocator* allocator, CFunction func)
    : GCObject(GCObjectType::Function), isC_(true), nupvalues_(0) // 初始化上值数量为0
      ,
      gclist_(nullptr) // 初始化GC链表指针为nullptr
      ,
      env_(nullptr) // 初始化环境表为nullptr
      ,
      cFunction_(func), apiCFunction_(nullptr), proto_(nullptr), upvalues_(LuaStdAllocator<Upvalue*>(allocator)) {
    if (func == nullptr) {
        throw std::invalid_argument("C function pointer cannot be null");
    }
}

Function::Function(ApiCFunction func) : Function(nullptr, func) {}

Function::Function(LuaAllocator* allocator, ApiCFunction func)
    : GCObject(GCObjectType::Function), isC_(true), nupvalues_(0), gclist_(nullptr), env_(nullptr), cFunction_(nullptr),
      apiCFunction_(func), proto_(nullptr), upvalues_(LuaStdAllocator<Upvalue*>(allocator)) {
    if (func == nullptr) {
        throw std::invalid_argument("Lua C API function pointer cannot be null");
    }
}

Function::Function(Proto* proto) : Function(nullptr, proto) {}

Function::Function(LuaAllocator* allocator, Proto* proto)
    : GCObject(GCObjectType::Function), isC_(false), nupvalues_(0) // 初始化上值数量为0
      ,
      gclist_(nullptr) // 初始化GC链表指针为nullptr
      ,
      env_(nullptr) // 初始化环境表为nullptr
      ,
      cFunction_(nullptr), apiCFunction_(nullptr), proto_(proto), upvalues_(LuaStdAllocator<Upvalue*>(allocator)) {
    if (proto == nullptr) {
        throw std::invalid_argument("Proto pointer cannot be null");
    }
}

Function::~Function() {
    // Proto和Upvalue由GC系统管理，这里不需要手动删除
}

i32 Function::callCFunction(LuaState* state) const {
    if (apiCFunction_ != nullptr) {
        return apiCFunction_(reinterpret_cast<lua_State*>(state));
    }
    if (cFunction_ != nullptr) {
        return cFunction_(state);
    }
    throw std::logic_error("Function does not contain a C callback");
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
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, upvalue);
    }
    upvalues_[index] = upvalue;
}

void Function::addUpvalue(Upvalue* upvalue) {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, upvalue);
    }
    upvalues_.push_back(upvalue);
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->accountObjectSizeChange(this);
    }
    // 同步nupvalues_字段（ClosureHeader字段）
    nupvalues_ = static_cast<u8>(upvalues_.size());
}

void Function::setEnv(Table* env) {
    if (GarbageCollector* gc = getOwnerCollector()) {
        gc->writeBarrier(this, env);
    }

    env_ = env;
}

// =====================================================================
// GC支持
// =====================================================================

void Function::mark(GarbageCollector& gc) {
    // 如果是Lua函数，标记函数原型
    if (!isC_) {
        gc.markObject(proto_);
    }

    // 标记所有upvalue
    for (Upvalue* uv : upvalues_) {
        gc.markObject(uv);
    }

    // 标记环境表（Lua 5.1兼容）
    gc.markObject(env_);
}

usize Function::getSize() const {
    // 基础大小 + upvalue数组大小
    return sizeof(Function) + upvalues_.capacity() * sizeof(Upvalue*);
}

} // namespace Lua
