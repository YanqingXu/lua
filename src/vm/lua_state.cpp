/**
 * @file lua_state.cpp
 * @brief Lua状态管理实现
 * 
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "vm/lua_state.hpp"

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
{
}

LuaState::~LuaState() {
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

} // namespace Lua

