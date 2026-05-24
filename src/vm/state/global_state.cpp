/**
 * @file global_state.cpp
 * @brief Lua全局状态管理实现
 *
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "vm/state/global_state.hpp"
#include "core/thread.hpp"
#include "vm/state/lua_state.hpp"
#include <cstring>  // for memset
#include <iostream> // for debug output

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
    : gc_()
    , stringPool_(StringPool::getInstance())
    , registry_(nullptr)
    , mainThread_(nullptr)
    , memerrmsg_(nullptr)
{
    gc_.setGlobalState(this);
    stringPool_.setGarbageCollector(&gc_);

    // 初始化元表数组为nullptr
    std::memset(metatables_, 0, sizeof(metatables_));

    // 初始化元方法名称数组为nullptr
    std::memset(tmname_, 0, sizeof(tmname_));

    // 子任务1.1：调整字符串池大小到初始值
    stringPool_.resize(32);

    // 子任务1.2：初始化元方法名称
    initMetamethodNames();

    // 子任务1.3：初始化保留字
    initReservedWords();

    // 子任务1.4：固定内存错误消息
    memerrmsg_ = stringPool_.intern("not enough memory");
    gc_.registerObject(memerrmsg_);
    memerrmsg_->markFixed();  // 标记为固定，防止在内存不足时被GC回收

    // 创建注册表
    registry_ = new Table();
    gc_.registerObject(registry_);
    registry_->setMarked(registry_->getMarked() | GCBits::FIXED);
    gc_.addRoot(registry_);  // 注册表永远不被回收
}

GlobalState::~GlobalState() {
    // 注意：不需要手动删除registry_，因为GC会处理
    // 但需要从根对象中移除
    if (registry_) {
        gc_.removeRoot(registry_);
    }
    stringPool_.setGarbageCollector(nullptr);
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

void GlobalState::markRoots(GarbageCollector& gc, LuaState* currentState) const {
    gc.markObject(registry_);
    gc.markObject(memerrmsg_);

    for (GCString* name : tmname_) {
        gc.markObject(name);
    }

    for (Table* metatable : metatables_) {
        gc.markObject(metatable);
    }

    if (currentState != nullptr) {
        gc.markState(currentState);
    }

    if (mainThread_ != nullptr && mainThread_ != currentState) {
        gc.markState(mainThread_);
    }

    gc.markObject(runningThread_);
}

// =====================================================================
// 元方法名称管理
// =====================================================================

/**
 * @brief 初始化元方法名称
 *
 * 创建并固定所有17个元方法名称字符串，防止GC回收。
 */
void GlobalState::initMetamethodNames() {
    // 元方法名称数组（与TMS枚举顺序一致）
    static const char* const metamethodNames[] = {
        "__index", "__newindex",
        "__gc", "__mode", "__eq",
        "__add", "__sub", "__mul", "__div", "__mod",
        "__pow", "__unm", "__len", "__lt", "__le",
        "__concat", "__call"
    };

    // 创建并固定所有元方法名称字符串
    for (usize i = 0; i < static_cast<usize>(TMS::TM_N); i++) {
        tmname_[i] = stringPool_.intern(metamethodNames[i]);
        gc_.registerObject(tmname_[i]);
        tmname_[i]->markFixed();  // 标记为固定，防止GC回收
    }
}

/**
 * @brief 获取元方法名称字符串
 */
GCString* GlobalState::getMetamethodName(TMS event) const noexcept {
    usize index = static_cast<usize>(event);
    if (index < static_cast<usize>(TMS::TM_N)) {
        return tmname_[index];
    }
    return nullptr;
}

/**
 * @brief 初始化保留字（关键字）
 *
 * 创建并固定所有21个Lua关键字字符串，防止GC回收。
 */
void GlobalState::initReservedWords() {
    // Lua 5.1的21个保留字（按字母顺序）
    static const char* const reservedWords[] = {
        "and", "break", "do", "else", "elseif",
        "end", "false", "for", "function", "if",
        "in", "local", "nil", "not", "or",
        "repeat", "return", "then", "true", "until",
        "while"
    };

    // 创建并固定所有保留字字符串
    for (const char* word : reservedWords) {
        GCString* str = stringPool_.intern(word);
        gc_.registerObject(str);
        str->markFixed();  // 标记为固定，防止GC回收
    }
}

} // namespace Lua

