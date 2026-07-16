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

    LuaVector<CallInfo>& callStack = state->getCallStack();
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

        if (local.startpc <= static_cast<i32>(pc) && static_cast<i32>(pc) < local.endpc) {
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
    LuaVector<CallInfo>& callStack = state->getCallStack();
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
            // Hooks can run between VM instructions while unnamed temporaries
            // are still live in registers. Normal GC uses LocVar ranges so
            // expired loop locals do not keep weak-table entries alive.
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
    return value.isString() || value.isTable() || value.isFunction() || value.isUserdata() || value.isThread();
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
    // Establish queue capacity before recolouring objects.  If allocation
    // fails, the previously completed collector state remains intact.
    grayList_.reserve(objectCount_);
    weakTables_.reserve(objectCount_);

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
        // Keep the object published in the queue until its complete child
        // graph has been scanned. A child push may exhaust spare capacity and
        // throw; retaining this slot makes rollback allocation-free.
        const usize objectIndex = grayList_.size() - 1;
        GCObject* obj = grayList_[objectIndex];

        // 标记为黑色并扫描其子图。If a child queue allocation fails, the
        // original queue slot remains present; changing the object back to
        // gray is enough to make a later retry safe and idempotent.
        obj->setColor(GCColor::Black);
        try {
            obj->mark(*this);
        } catch (...) {
            obj->setColor(GCColor::Gray);
            throw;
        }

        // Children may have been appended (and the vector may have moved),
        // so remove the original slot by index rather than by reference.
        grayList_[objectIndex] = grayList_.back();
        grayList_.pop_back();
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
        const usize publishedIndex = externalMarked_.size();
        externalMarked_.push_back(obj);
        try {
            obj->mark(*this);
        } catch (...) {
            // A later gray/external queue growth may fail while scanning this
            // graph. Remove only this incomplete publication so a retry scans
            // it again; successfully completed nested entries remain valid.
            using Difference = LuaVector<GCObject*>::difference_type;
            externalMarked_.erase(externalMarked_.begin() + static_cast<Difference>(publishedIndex));
            throw;
        }
        return;
    }

    // 如果已经是灰色或黑色，不需要重复标记
    if (obj->getColor() != GCColor::White) {
        return;
    }

    // Publish the queue entry before changing color. If allocation fails,
    // the object remains white and a later collection can retry it.
    grayList_.push_back(obj);
    obj->setColor(GCColor::Gray);
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

void GarbageCollector::writeBarrierDeferredNoexcept(GCObject* owner, const Value& value) noexcept {
    if (!valueContainsObject(value)) {
        return;
    }

    GCObject* child = objectFromValue(value);
    if (owner == nullptr || child == nullptr || owner->getOwnerCollector() != this ||
        child->getOwnerCollector() != this || !owner->isBlack() || !child->isWhite()) {
        return;
    }

    if (incrementalPhase_ == IncrementalPhase::Sweep) {
        // Sweep never returns to propagation.  Abandon the remaining cursor
        // so the newly closed upvalue and its complete child graph are marked
        // together by the next cycle.
        resetIncrementalCycle();
        return;
    }

    if (grayList_.size() >= grayList_.capacity()) {
        // Continuing toward sweep would violate the tri-colour invariant.
        // Dropping the unfinished cycle is safe: no white object has been
        // reclaimed yet, and the next step starts a fresh mark phase.
        resetIncrementalCycle();
        return;
    }

    // Capacity was reserved at the start of the mark phase.  Pointer moves
    // and size growth are non-throwing when no reallocation is required.
    grayList_.push_back(child);
    child->setColor(GCColor::Gray);
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

    markObject(state->getGlobalTable());

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
