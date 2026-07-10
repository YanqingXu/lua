/**
 * @file gc_finalize.cpp
 * @brief 垃圾回收器终结器处理实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/userdata.hpp"
#include "core/value.hpp"
#include "vm/state/global_state.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/stack.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/vm.hpp"
#include <exception>

namespace Lua {

Value GarbageCollector::getFinalizer(Userdata* userdata) const {
    if (userdata == nullptr || userdata->getMetatable() == nullptr) {
        return Value();
    }

    GlobalState& state = globalState_ != nullptr ? *globalState_ : GlobalState::getInstance();
    GCString* gcName = state.getMetamethodName(TMS::TM_GC);
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
    LuaVector<Userdata*> finalizers(pendingFinalizers_.get_allocator());
    finalizers.swap(pendingFinalizers_);

    Stack& stack = state->getStack();
    for (usize i = 0; i < finalizers.size(); i++) {
        Userdata* userdata = finalizers[i];
        Value finalizer = getFinalizer(userdata);
        if (finalizer.isNil()) {
            continue;
        }

        usize savedTop = state->getAbsoluteTop();
        usize savedStackTop = stack.size();
        usize savedCI = state->getCurrentCI();
        std::exception_ptr finalizerError;

        try {
            stack.setTop(savedTop);
            state->setAbsoluteTop(savedTop);
            state->pushValue(finalizer);
            state->pushUserdata(userdata);
            RuntimeServices services(state->getGlobalState());
            VM::call(services, state, 1, 0);
        } catch (...) {
            finalizerError = std::current_exception();
        }

        while (state->getCurrentCI() > savedCI) {
            state->popCallInfo();
        }
        stack.setTop(savedStackTop);
        state->setAbsoluteTop(savedTop);

        if (finalizerError) {
            for (usize j = i + 1; j < finalizers.size(); j++) {
                pendingFinalizers_.push_back(finalizers[j]);
            }
            finalizersRunning_ = false;
            std::rethrow_exception(finalizerError);
        }
    }

    finalizersRunning_ = false;
}

} // namespace Lua
