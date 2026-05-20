/**
 * @file gc_mark.cpp
 * @brief 垃圾回收器标记阶段实现
 */

#include "gc/garbage_collector.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/thread.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "core/value.hpp"
#include "vm/global_state.hpp"
#include "vm/lua_state.hpp"
#include <algorithm>

namespace Lua {

bool GarbageCollector::valueContainsObject(const Value& value) {
    return value.isString() || value.isTable() || value.isFunction() ||
           value.isUserdata() || value.isThread();
}

GCObject* GarbageCollector::objectFromValue(const Value& value) {
    if (value.isString()) {
        return value.asString();
    }
    if (value.isTable()) {
        return value.asTable();
    }
    if (value.isFunction()) {
        return value.asFunction();
    }
    if (value.isUserdata()) {
        return value.asUserdata();
    }
    if (value.isThread()) {
        return value.asThread();
    }
    return nullptr;
}

void GarbageCollector::mark() {
    mark(nullptr);
}

void GarbageCollector::mark(LuaState* currentState) {
    // 1. 重置所有对象为白色（保留FIXED和FINALIZED，清除上一轮弱表模式）
    GCObject* obj = allObjects_;
    while (obj != nullptr) {
        u8 preserved = obj->getMarked() & (GCBits::FIXED | GCBits::FINALIZED);
        obj->setMarked(preserved);

        // 设置为白色
        obj->setColor(GCColor::White);

        obj = obj->getNext();
    }

    // 2. 清空本轮临时列表
    grayList_.clear();
    weakTables_.clear();

    // 3. 标记所有根对象为灰色
    for (GCObject* root : roots_) {
        if (root != nullptr) {
            markObject(root);
        }
    }

    // 4. 标记当前执行状态及全局状态中的共享根
    if (currentState != nullptr) {
        currentState->getGlobalState().markRoots(*this, currentState);
    }

    // 终结器队列中的 userdata 已经被复活，必须在真正运行 __gc 前保持存活。
    for (Userdata* userdata : pendingFinalizers_) {
        markObject(userdata);
    }

    // 5. 传播标记
    propagateMarks();
}

void GarbageCollector::propagateMarks() {
    // 处理所有灰色对象
    while (!grayList_.empty()) {
        // 取出一个灰色对象
        GCObject* obj = grayList_.back();
        grayList_.pop_back();
        
        // 标记为黑色
        obj->setColor(GCColor::Black);
        
        // 调用对象的mark方法，由对象通过gc.markObject/markValue报告引用关系。
        obj->mark(*this);
    }
}

void GarbageCollector::markObject(GCObject* obj) {
    if (obj == nullptr) {
        return;
    }
    
    // 如果已经是灰色或黑色，不需要重复标记
    if (obj->getColor() != GCColor::White) {
        return;
    }
    
    // 标记为灰色
    obj->setColor(GCColor::Gray);
    
    // 添加到灰色列表
    grayList_.push_back(obj);
}

void GarbageCollector::markValue(const Value& value) {
    if (valueContainsObject(value)) {
        markObject(objectFromValue(value));
    }
}

void GarbageCollector::markState(LuaState* state) {
    if (state == nullptr) {
        return;
    }

    Stack& stack = state->getStack();
    usize scanTop = std::min(state->getAbsoluteTop(), stack.size());

    Vec<CallInfo>& callStack = state->getCallStack();
    usize callStackSize = state->getCallStackSize();
    for (usize i = 0; i < callStackSize && i < callStack.size(); i++) {
        scanTop = std::max(scanTop, std::min(callStack[i].top, stack.size()));
        if (callStack[i].func < stack.size()) {
            markValue(stack.at(callStack[i].func));
        }
    }

    for (usize i = 0; i < scanTop; i++) {
        markValue(stack.at(i));
    }

    Upvalue* uv = state->getOpenUpvalues();
    while (uv != nullptr) {
        markObject(uv);
        uv = uv->getNext();
    }

    markObject(state->getDebugHook());
}

} // namespace Lua
