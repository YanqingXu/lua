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
#include <algorithm>
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
        if (obj->getType() == GCObjectType::Userdata && obj->getColor() == GCColor::White &&
            (obj->getMarked() & (GCBits::FIXED | GCBits::FINALIZED)) == 0) {
            auto* userdata = static_cast<Userdata*>(obj);
            Value finalizer = getFinalizer(userdata);
            if (!finalizer.isNil()) {
                pendingFinalizers_.push_back(userdata);
                // Queue publication can allocate. Mark FINALIZED only after
                // it succeeds so OOM cannot silently discard __gc forever.
                obj->setMarked(obj->getMarked() | GCBits::FINALIZED);
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

    // Keep the scheduled userdata in pendingFinalizers_ while callbacks run.
    // A __gc callback may invoke collectgarbage recursively; the mark phase
    // treats this member queue as roots. Moving the queue into a local vector
    // made the remaining callbacks invisible to a nested collection and left
    // dangling pointers in the outer finalizer loop.
    LuaVector<Userdata*> finalizers(pendingFinalizers_.begin(), pendingFinalizers_.end(),
                                    pendingFinalizers_.get_allocator());
    // Copying can allocate. Do not publish the reentrancy guard until that
    // succeeds, otherwise an OOM would permanently suppress finalizers.
    finalizersRunning_ = true;

    Stack& stack = state->getStack();
    for (usize i = 0; i < finalizers.size(); i++) {
        Userdata* userdata = finalizers[i];
        Value finalizer = getFinalizer(userdata);
        if (finalizer.isNil()) {
            pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                     pendingFinalizers_.end());
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
        // The physical stack can reserve a wider helper window than the
        // state's logical top.  The finalizer function and userdata argument
        // were written into that window; merely lowering the logical top
        // leaves them visible to the next wide root scan.  Clear every slot
        // that belonged to the saved helper window before restoring it.
        for (usize slot = savedTop; slot < savedStackTop; ++slot) {
            stack[slot] = Value();
        }
        stack.setTop(savedStackTop);
        state->setAbsoluteTop(savedTop);

        pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                 pendingFinalizers_.end());

        if (finalizerError) {
            finalizersRunning_ = false;
            std::rethrow_exception(finalizerError);
        }
    }

    finalizersRunning_ = false;
}

} // namespace Lua
