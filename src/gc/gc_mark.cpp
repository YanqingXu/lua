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
#include "vm/state/global_state.hpp"
#include "vm/state/lua_state.hpp"
#include <algorithm>

namespace Lua {

namespace {

void markWideStackRoots(GarbageCollector& gc, LuaState* state) {
    Stack& stack = state->getStack();
    usize scanTop = std::min(state->getAbsoluteTop(), stack.size());

    Vec<CallInfo>& callStack = state->getCallStack();
    usize callStackSize = state->getCallStackSize();
    for (usize i = 0; i < callStackSize && i < callStack.size(); i++) {
        scanTop = std::max(scanTop, std::min(callStack[i].top, stack.size()));
        if (callStack[i].func < stack.size()) {
            gc.markValue(stack.at(callStack[i].func));
        }
    }

    for (usize i = 0; i < scanTop; i++) {
        gc.markValue(stack.at(i));
    }
}

Function* frameFunction(Stack& stack, const CallInfo& ci) {
    if (ci.func >= stack.size()) {
        return nullptr;
    }

    Value& functionValue = stack.at(ci.func);
    return functionValue.isFunction() ? functionValue.asFunction() : nullptr;
}

usize frameCurrentPc(const CallInfo& ci, Proto* proto) {
    if (proto == nullptr || ci.savedpc == nullptr) {
        return 0;
    }

    const auto code = proto->getInstructionSpan();
    if (code.empty()) {
        return 0;
    }

    const Instruction* begin = code.data();
    const Instruction* end = begin + code.size();
    if (ci.savedpc <= begin) {
        return 0;
    }
    if (ci.savedpc > end) {
        return code.size() - 1;
    }

    return static_cast<usize>((ci.savedpc - begin) - 1);
}

void markLuaFrameLocals(GarbageCollector& gc, LuaState* state, const CallInfo& ci, Proto* proto) {
    Stack& stack = state->getStack();
    usize pc = frameCurrentPc(ci, proto);

    for (usize i = 0; i < proto->getLocVarCount(); i++) {
        const LocVar& local = proto->getLocVar(i);
        if (local.reg < 0) {
            continue;
        }

        usize slot = ci.base + static_cast<usize>(local.reg);
        if (slot >= stack.size()) {
            continue;
        }

        if (local.startpc <= static_cast<i32>(pc) &&
            static_cast<i32>(pc) < local.endpc) {
            gc.markValue(stack.at(slot));
        }
    }
}

void markLuaFrameStackWindow(GarbageCollector& gc, LuaState* state, const CallInfo& ci) {
    Stack& stack = state->getStack();
    usize top = std::min(ci.top, stack.size());
    usize base = std::min(ci.base, top);

    for (usize i = base; i < top; i++) {
        gc.markValue(stack.at(i));
    }
}

void markCFrameStackWindow(GarbageCollector& gc, LuaState* state, const CallInfo& ci) {
    Stack& stack = state->getStack();
    usize absTop = std::min(state->getAbsoluteTop(), stack.size());
    usize top = std::min({ci.top, absTop, stack.size()});

    for (usize i = ci.base; i < top; i++) {
        gc.markValue(stack.at(i));
    }
}

void markPreciseStackRoots(GarbageCollector& gc, LuaState* state) {
    Stack& stack = state->getStack();
    Vec<CallInfo>& callStack = state->getCallStack();
    usize callStackSize = state->getCallStackSize();

    if (callStackSize == 0 || callStack.empty()) {
        usize scanTop = std::min(state->getAbsoluteTop(), stack.size());
        for (usize i = 0; i < scanTop; i++) {
            gc.markValue(stack.at(i));
        }
        return;
    }

    for (usize i = 0; i < callStackSize && i < callStack.size(); i++) {
        const CallInfo& ci = callStack[i];
        if (ci.func < stack.size()) {
            gc.markValue(stack.at(ci.func));
        }

        Function* function = frameFunction(stack, ci);
        Proto* proto = function != nullptr ? function->getProto() : nullptr;
        if (proto != nullptr) {
            // Debug locals are not a liveness map. Hooks may run collection
            // while the interrupted Lua frame still has live register values.
            if (state->isDebugHookActive()) {
                markLuaFrameStackWindow(gc, state, ci);
            }
            markLuaFrameLocals(gc, state, ci, proto);
        } else {
            markCFrameStackWindow(gc, state, ci);
        }
    }
}

} // namespace

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
    externalMarked_.clear();

    // 3. 标记所有根对象为灰色
    for (GCObject* root : roots_) {
        if (root != nullptr) {
            markObject(root);
        }
    }

    // 4. 标记当前执行状态及全局状态中的共享根
    if (currentState != nullptr) {
        currentState->getGlobalState().markRoots(*this, currentState);
    } else if (globalState_ != nullptr) {
        globalState_->markRoots(*this, nullptr);
    }

    // 终结器队列中的 userdata 已经被复活，必须在真正运行 __gc 前保持存活。
    for (Userdata* userdata : pendingFinalizers_) {
        markObject(userdata);
    }

    // 5. 传播标记
    propagateMarks();
}

void GarbageCollector::propagateMarks() {
    [[maybe_unused]] const usize propagated = propagateMarks(static_cast<usize>(-1));
}

usize GarbageCollector::propagateMarks(usize budget) {
    usize processed = 0;

    // 处理所有灰色对象
    while (!grayList_.empty() && processed < budget) {
        // 取出一个灰色对象
        GCObject* obj = grayList_.back();
        grayList_.pop_back();
        
        // 标记为黑色
        obj->setColor(GCColor::Black);
        
        // 调用对象的mark方法，由对象通过gc.markObject/markValue报告引用关系。
        obj->mark(*this);
        ++processed;
    }

    return processed;
}

void GarbageCollector::markObject(GCObject* obj) {
    if (obj == nullptr) {
        return;
    }

    GarbageCollector* owner = obj->getOwnerCollector();
    if (owner != nullptr && owner != this) {
        if (std::find(externalMarked_.begin(), externalMarked_.end(), obj) != externalMarked_.end()) {
            return;
        }
        externalMarked_.push_back(obj);
        obj->mark(*this);
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

void GarbageCollector::writeBarrier(GCObject* owner, GCObject* child) {
    if (owner == nullptr || child == nullptr) {
        return;
    }

    if (owner->getOwnerCollector() != this || child->getOwnerCollector() != this) {
        return;
    }

    if (!owner->isBlack() || !child->isWhite()) {
        return;
    }

    markObject(child);
    propagateMarks();
}

void GarbageCollector::writeBarrier(GCObject* owner, const Value& value) {
    if (!valueContainsObject(value)) {
        return;
    }

    writeBarrier(owner, objectFromValue(value));
}

void GarbageCollector::writeRootBarrier(GCObject* child) {
    if (child == nullptr || child->getOwnerCollector() != this || !child->isWhite()) {
        return;
    }

    markObject(child);
    propagateMarks();
}

void GarbageCollector::markState(LuaState* state) {
    if (state == nullptr) {
        return;
    }

    if (preciseStackRoots_) {
        markPreciseStackRoots(*this, state);
    } else {
        markWideStackRoots(*this, state);
    }

    Upvalue* uv = state->getOpenUpvalues();
    while (uv != nullptr) {
        markObject(uv);
        uv = uv->getNext();
    }

    markObject(state->getDebugHook());
}

} // namespace Lua
