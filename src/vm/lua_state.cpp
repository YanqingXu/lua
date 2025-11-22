/**
 * @file lua_state.cpp
 * @brief Lua状态管理实现
 *
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "vm/lua_state.hpp"
#include "core/upvalue.hpp"

#ifdef DEBUG
#include <iostream>
#include <cassert>
#endif

namespace Lua {

// =====================================================================
// 静态工厂方法
// =====================================================================

LuaState* LuaState::newState() {
    LuaState* L = new LuaState();
    L->initialize();
    return L;
}

// =====================================================================
// 构造函数和析构函数
// =====================================================================

LuaState::LuaState()
    : globalState_(GlobalState::getInstance())
    , stack_(Stack::INITIAL_STACK_SIZE)
    , callStack_(INITIAL_CI_SIZE)
    , currentCI_(0)
    , globalTable_(nullptr)
    , status_(ThreadStatus::OK)
    , openUpvalues_(nullptr)
{
}

LuaState::~LuaState() {
    // 关闭所有open upvalue
    closeUpvalues(0);

    // 清理全局表（如果不是主线程的全局表）
    // 注意：全局表由GC管理，这里只需要移除根引用
    if (globalTable_) {
        globalState_.getGC().removeRoot(globalTable_);
    }
}

// =====================================================================
// 初始化
// =====================================================================

void LuaState::initialize() {
    // 创建全局表
    globalTable_ = new Table();
    globalState_.getGC().registerObject(globalTable_);
    globalState_.getGC().addRoot(globalTable_);
    
    // 初始化第一个调用信息（虚拟的主函数）
    CallInfo& ci = callStack_[0];
    ci.func = 0;
    ci.base = 0;
    ci.top = Stack::MIN_STACK_SIZE;
    ci.savedpc = nullptr;
    ci.nresults = MULTRET;
    ci.tailcalls = 0;
    
    // 在栈上放置一个nil值作为虚拟函数
    stack_.push(Value());  // nil
    
    // 如果这是第一个LuaState，设置为主线程
    if (globalState_.getMainThread() == nullptr) {
        globalState_.setMainThread(this);
    }
}

// =====================================================================
// Upvalue管理
// =====================================================================

Upvalue* LuaState::findOrCreateUpvalue(usize stackIndex) {
    // 1. 在链表中查找已存在的upvalue（按栈索引降序）
    Upvalue* prev = nullptr;
    Upvalue* curr = openUpvalues_;

    while (curr != nullptr && curr->getStackIndex() > stackIndex) {
        prev = curr;
        curr = curr->getNext();
    }

    // 2. 找到了，直接返回
    if (curr != nullptr && curr->getStackIndex() == stackIndex) {
        return curr;
    }

    // 3. 没找到，创建新的upvalue
    // ✅ 改进：只传递索引，不传递指针
    Upvalue* newUpval = Upvalue::createOpen(stackIndex);

    // 4. 插入链表（保持降序）
    newUpval->setNext(curr);
    if (prev == nullptr) {
        // 插入到链表头
        openUpvalues_ = newUpval;
    } else {
        // 插入到prev之后
        prev->setNext(newUpval);
    }

    // 5. 注册到GC
    globalState_.getGC().registerObject(newUpval);

    return newUpval;
}

void LuaState::closeUpvalues(usize level) {
    // 关闭所有栈索引 >= level 的upvalue
    while (openUpvalues_ != nullptr &&
           openUpvalues_->getStackIndex() >= level) {
        Upvalue* uv = openUpvalues_;
        openUpvalues_ = uv->getNext();  // 从链表移除
        uv->close(stack_);               // ✅ 改进：传入stack_引用
        uv->setNext(nullptr);            // 清除链表指针
    }
}

// =====================================================================
// CallInfo管理
// =====================================================================

CallInfo& LuaState::pushCallInfo() {
    // ✅ 改进：检查最大调用深度
    if (currentCI_ + 1 >= MAX_CALL_DEPTH) {
        throw std::runtime_error(
            "stack overflow: maximum call depth exceeded (limit: " +
            std::to_string(MAX_CALL_DEPTH) + ")"
        );
    }

    // 检查是否需要扩展调用栈
    if (currentCI_ + 1 >= callStack_.size()) {
        // ✅ 改进：双倍扩展，但不超过最大限制
        usize newSize = std::min(callStack_.size() * 2, MAX_CALL_DEPTH);
        callStack_.resize(newSize);
    }

    // 移动到下一个CallInfo
    currentCI_++;

    // 重置新的CallInfo
    callStack_[currentCI_].reset();

    return callStack_[currentCI_];
}

void LuaState::popCallInfo() {
    if (currentCI_ == 0) {
        throw std::runtime_error("LuaState::popCallInfo: cannot pop base CallInfo");
    }

    // ✅ 改进：清理当前CallInfo（调试模式）
    #ifdef DEBUG
    callStack_[currentCI_].reset();
    #endif

    currentCI_--;
}

// =====================================================================
// ✅ 改进：调试支持
// =====================================================================

#ifdef DEBUG
void LuaState::dumpCallStack() const {
    std::cout << "========================================\n";
    std::cout << "Call Stack Dump\n";
    std::cout << "========================================\n";
    std::cout << "Depth: " << (currentCI_ + 1) << " / " << MAX_CALL_DEPTH << "\n";
    std::cout << "----------------------------------------\n";

    for (usize i = 0; i <= currentCI_; i++) {
        std::cout << "  [" << i << "] " << callStack_[i].toString() << "\n";
    }

    std::cout << "========================================\n";
}

void LuaState::validateCallStack() const {
    assert(currentCI_ < callStack_.size());
    assert(currentCI_ < MAX_CALL_DEPTH);

    // 验证每个CallInfo
    for (usize i = 0; i <= currentCI_; i++) {
        callStack_[i].validate(stack_.size());
    }
}
#endif

// =====================================================================
// 栈操作
// =====================================================================

void LuaState::setTop(i32 idx) {
    i32 newTop;
    if (idx >= 0) {
        newTop = idx;
    } else {
        newTop = static_cast<i32>(stack_.size()) + idx + 1;
    }

    if (newTop < 0) {
        throw std::runtime_error("invalid stack index");
    }

    // 填充nil值或收缩栈
    while (static_cast<i32>(stack_.size()) < newTop) {
        stack_.push(Value());  // nil
    }
    while (static_cast<i32>(stack_.size()) > newTop) {
        stack_.pop();
    }
}

void LuaState::pushValue(i32 idx) {
    stack_.push(at(idx));
}

Value& LuaState::at(i32 idx) {
    if (idx > 0) {
        // 正索引：从栈底开始（1-based）
        if (idx > static_cast<i32>(stack_.size())) {
            throw std::out_of_range("stack index out of range");
        }
        return stack_.at(static_cast<usize>(idx - 1));
    } else if (idx < 0) {
        // 负索引：从栈顶倒数
        i32 absIdx = static_cast<i32>(stack_.size()) + idx;
        if (absIdx < 0) {
            throw std::out_of_range("stack index out of range");
        }
        return stack_.at(static_cast<usize>(absIdx));
    } else {
        throw std::invalid_argument("stack index cannot be 0");
    }
}

const Value& LuaState::at(i32 idx) const {
    return const_cast<LuaState*>(this)->at(idx);
}

// =====================================================================
// 全局变量操作
// =====================================================================

void LuaState::setGlobal(const Str& name, const Value& value) {
    if (!globalTable_) {
        throw std::runtime_error("global table not initialized");
    }

    // 创建字符串键
    GCString* key = globalState_.getStringPool().intern(name);
    globalTable_->set(Value(key), value);
}

Value LuaState::getGlobal(const Str& name) {
    if (!globalTable_) {
        throw std::runtime_error("global table not initialized");
    }

    // 创建字符串键
    GCString* key = globalState_.getStringPool().intern(name);
    return globalTable_->get(Value(key));
}

// =====================================================================
// 类型检查
// =====================================================================

bool LuaState::isNumber(i32 idx) const {
    try {
        return at(idx).isNumber();
    } catch (...) {
        return false;
    }
}

bool LuaState::isString(i32 idx) const {
    try {
        return at(idx).isString();
    } catch (...) {
        return false;
    }
}

bool LuaState::isTable(i32 idx) const {
    try {
        return at(idx).isTable();
    } catch (...) {
        return false;
    }
}

bool LuaState::isFunction(i32 idx) const {
    try {
        return at(idx).isFunction();
    } catch (...) {
        return false;
    }
}

bool LuaState::isNil(i32 idx) const {
    try {
        return at(idx).isNil();
    } catch (...) {
        return true;  // 无效索引视为nil
    }
}

bool LuaState::isBoolean(i32 idx) const {
    try {
        return at(idx).isBoolean();
    } catch (...) {
        return false;
    }
}

i32 LuaState::type(i32 idx) const {
    try {
        const Value& v = at(idx);
        if (v.isNil()) return 0;        // LUA_TNIL
        if (v.isBoolean()) return 1;    // LUA_TBOOLEAN
        if (v.isNumber()) return 3;     // LUA_TNUMBER
        if (v.isString()) return 4;     // LUA_TSTRING
        if (v.isTable()) return 5;      // LUA_TTABLE
        if (v.isFunction()) return 6;   // LUA_TFUNCTION
        if (v.isUserdata()) return 7;   // LUA_TUSERDATA
        return -1;  // LUA_TNONE
    } catch (...) {
        return -1;  // LUA_TNONE
    }
}

const char* LuaState::typeName(i32 tp) const {
    static const char* typeNames[] = {
        "nil", "boolean", "lightuserdata", "number",
        "string", "table", "function", "userdata", "thread"
    };
    if (tp >= 0 && tp < 9) {
        return typeNames[tp];
    }
    return "no value";
}

// =====================================================================
// 类型转换
// =====================================================================

LuaNumber LuaState::toNumber(i32 idx) const {
    try {
        const Value& v = at(idx);
        if (v.isNumber()) {
            return v.asNumber();
        }
        // TODO: 字符串到数字的转换
        return 0.0;
    } catch (...) {
        return 0.0;
    }
}

const char* LuaState::toString(i32 idx) {
    try {
        Value& v = at(idx);
        if (v.isString()) {
            return v.asString()->c_str();
        }
        if (v.isNumber()) {
            // 将数字转换为字符串并替换栈上的值
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.14g", v.asNumber());
            GCString* str = globalState_.getStringPool().intern(buffer);
            v = Value(str);
            return str->c_str();
        }
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

bool LuaState::toBoolean(i32 idx) const {
    try {
        const Value& v = at(idx);
        // Lua中只有nil和false是假值
        if (v.isNil()) return false;
        if (v.isBoolean()) return v.asBoolean();
        return true;  // 其他所有值都是真值
    } catch (...) {
        return false;
    }
}

// =====================================================================
// 元表操作
// =====================================================================

bool LuaState::getMetatable(i32 idx) {
    try {
        const Value& v = at(idx);
        if (v.isTable()) {
            Table* mt = v.asTable()->getMetatable();
            if (mt) {
                pushTable(mt);
                return true;
            }
        }
        // TODO: 支持其他类型的元表
        return false;
    } catch (...) {
        return false;
    }
}

bool LuaState::setMetatable(i32 idx) {
    try {
        Value& v = at(idx);
        if (!v.isTable()) {
            return false;  // 只能为表设置元表
        }

        Value& mt = top();
        if (mt.isNil()) {
            v.asTable()->setMetatable(nullptr);
        } else if (mt.isTable()) {
            v.asTable()->setMetatable(mt.asTable());
        } else {
            return false;  // 元表必须是表或nil
        }

        pop();  // 弹出元表
        return true;
    } catch (...) {
        return false;
    }
}

// =====================================================================
// 错误处理
// =====================================================================

void LuaState::error(const char* msg) {
    setStatus(ThreadStatus::ErrRun);
    throw std::runtime_error(msg);
}

i32 LuaState::error() {
    setStatus(ThreadStatus::ErrRun);
    try {
        const char* msg = toString(-1);
        if (msg) {
            throw std::runtime_error(msg);
        } else {
            throw std::runtime_error("error object is not a string");
        }
    } catch (...) {
        throw std::runtime_error("unknown error");
    }
}

} // namespace Lua

