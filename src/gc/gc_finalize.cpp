/**
 * @file gc_finalize.cpp
 * @brief 垃圾回收器终结器处理实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/userdata.hpp"
#include "core/value.hpp"
#include "vm/global_state.hpp"
#include "vm/lua_state.hpp"
#include "vm/stack.hpp"
#include "vm/vm.hpp"

namespace Lua {

Value GarbageCollector::getFinalizer(Userdata* userdata) const {
    if (userdata == nullptr || userdata->getMetatable() == nullptr) {
        return Value();
    }

    GCString* gcName = GlobalState::getInstance().getMetamethodName(TMS::TM_GC);
    return userdata->getMetatable()->get(Value(gcName));
}

void GarbageCollector::prepareFinalizers() {
    GCObject* obj = allObjects_;
    while (obj != nullptr) {
        if (obj->getType() == GCObjectType::Userdata &&
            obj->getColor() == GCColor::White &&
            (obj->getMarked() & (GCBits::FIXED | GCBits::FINALIZED)) == 0) {
            auto* userdata = static_cast<Userdata*>(obj);
            Value finalizer = getFinalizer(userdata);
            if (!finalizer.isNil()) {
                obj->setMarked(obj->getMarked() | GCBits::FINALIZED);
                pendingFinalizers_.push_back(userdata);
                markObject(obj);
            }
        }

        obj = obj->getNext();
    }
}

void GarbageCollector::runFinalizers(LuaState* state) {
    if (state == nullptr || finalizersRunning_ || pendingFinalizers_.empty()) {
        return;
    }

    finalizersRunning_ = true;
    Vec<Userdata*> finalizers;
    finalizers.swap(pendingFinalizers_);

    Stack& stack = state->getStack();
    for (Userdata* userdata : finalizers) {
        Value finalizer = getFinalizer(userdata);
        if (finalizer.isNil()) {
            continue;
        }

        usize savedTop = state->getAbsoluteTop();
        usize savedStackTop = stack.size();
        usize savedCI = state->getCurrentCI();

        try {
            stack.setTop(savedTop);
            state->setAbsoluteTop(savedTop);
            state->pushValue(finalizer);
            state->pushUserdata(userdata);
            VM::call(state, 1, 0);
        } catch (...) {
            // Lua 5.1 的 GC 终结流程不应让单个 finalizer 错误打断整轮回收。
        }

        while (state->getCurrentCI() > savedCI) {
            state->popCallInfo();
        }
        stack.setTop(savedStackTop);
        state->setAbsoluteTop(savedTop);
    }

    finalizersRunning_ = false;
}

} // namespace Lua
