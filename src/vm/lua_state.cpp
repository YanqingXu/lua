/**
 * @file lua_state.cpp
 * @brief Lua状态管理实现
 *
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "vm/lua_state.hpp"
#include "core/upvalue.hpp"

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
    Value* stackValue = &stack_.at(stackIndex);
    Upvalue* newUpval = Upvalue::createOpen(stackValue, stackIndex);

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
        uv->close();                     // 关闭（复制值）
        uv->setNext(nullptr);            // 清除链表指针
    }
}

} // namespace Lua

