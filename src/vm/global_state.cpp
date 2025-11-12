/**
 * @file global_state.cpp
 * @brief Lua全局状态管理实现
 * 
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "vm/global_state.hpp"
#include <cstring>  // for memset

namespace Lua {

// =====================================================================
// 单例实现
// =====================================================================

GlobalState& GlobalState::getInstance() {
    static GlobalState instance;
    return instance;
}

// =====================================================================
// 构造函数和析构函数
// =====================================================================

GlobalState::GlobalState()
    : stringPool_(StringPool::getInstance())
    , gc_(GarbageCollector::getInstance())
    , registry_(nullptr)
    , mainThread_(nullptr)
{
    // 初始化元表数组为nullptr
    std::memset(metatables_, 0, sizeof(metatables_));
    
    // 创建注册表
    registry_ = new Table();
    gc_.registerObject(registry_);
    gc_.addRoot(registry_);  // 注册表永远不被回收
}

GlobalState::~GlobalState() {
    // 注意：不需要手动删除registry_，因为GC会处理
    // 但需要从根对象中移除
    if (registry_) {
        gc_.removeRoot(registry_);
    }
}

// =====================================================================
// 元表管理
// =====================================================================

Table* GlobalState::getMetatable(ValueType type) const noexcept {
    usize index = static_cast<usize>(type);
    if (index < 9) {
        return metatables_[index];
    }
    return nullptr;
}

void GlobalState::setMetatable(ValueType type, Table* metatable) noexcept {
    usize index = static_cast<usize>(type);
    if (index < 9) {
        metatables_[index] = metatable;
    }
}

} // namespace Lua

