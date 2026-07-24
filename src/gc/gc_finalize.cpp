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

    for (usize i = 0; i < finalizers.size(); i++) {
        Userdata* userdata = finalizers[i];
        Value finalizer = getFinalizer(userdata);
        if (finalizer.isNil()) {
            pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                     pendingFinalizers_.end());
            continue;
        }

        try {
            callFinalizer(state, userdata);
        } catch (...) {
            pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                     pendingFinalizers_.end());
            finalizersRunning_ = false;
            throw;
        }

        pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                 pendingFinalizers_.end());
    }

    finalizersRunning_ = false;
}

void GarbageCollector::callFinalizer(LuaState* state, Userdata* userdata) {
    Value finalizer = getFinalizer(userdata);
    if (finalizer.isNil()) {
        return;
    }

    Stack& stack = state->getStack();
    const usize savedTop = state->getAbsoluteTop();
    const usize savedStackTop = stack.size();
    const usize savedCI = state->getCurrentCI();
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
    // The physical stack can reserve a wider helper window than the state's
    // logical top.  Clear the callback window before restoring it so a later
    // wide root scan cannot retain the finalizer or its userdata argument.
    const usize clearTop = std::max(savedStackTop, stack.size());
    for (usize slot = savedTop; slot < clearTop; ++slot) {
        stack[slot] = Value();
    }
    stack.setTop(savedStackTop);
    state->setAbsoluteTop(savedTop);

    if (finalizerError) {
        std::rethrow_exception(finalizerError);
    }
}

void GarbageCollector::finalizeAll(LuaState* state) noexcept {
    if (state == nullptr || finalizersRunning_) {
        return;
    }

    resetIncrementalCycle();

    // lua_close does not preserve an executing frame.  Closing open upvalues
    // before clearing the stack matches Lua 5.1 and leaves the already-owned
    // base-frame storage available even under persistent allocator failure.
    try {
        state->closeUpvalues(0);
    } catch (...) {
        // Shutdown must continue and release the runtime even if a damaged
        // state cannot close one of its upvalues cleanly.
    }
    while (state->getCurrentCI() > 0) {
        try {
            state->popCallInfo();
        } catch (...) {
            break;
        }
    }

    Stack& stack = state->getStack();
    for (usize slot = 0; slot < stack.capacity(); ++slot) {
        stack[slot] = Value();
    }
    stack.clear();
    state->setAbsoluteTop(0);
    (void)state->tryPushValueNoAlloc(Value());
    state->setStatus(ThreadStatus::OK);

    finalizersRunning_ = true;
    try {
        for (;;) {
            Userdata* userdata = nullptr;
            if (!pendingFinalizers_.empty()) {
                userdata = pendingFinalizers_.back();
                pendingFinalizers_.pop_back();
            } else {
                // Select only one object per pass.  A finalizer may run a
                // nested collection and mutate the intrusive object list.
                for (GCObject* object = allObjects_; object != nullptr; object = object->getNext()) {
                    if (object->getType() != GCObjectType::Userdata || (object->getMarked() & GCBits::FINALIZED) != 0) {
                        continue;
                    }
                    userdata = static_cast<Userdata*>(object);
                    object->setMarked(object->getMarked() | GCBits::FINALIZED);
                    break;
                }
            }

            if (userdata == nullptr) {
                break;
            }

            pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                     pendingFinalizers_.end());
            try {
                callFinalizer(state, userdata);
            } catch (...) {
                // Lua 5.1 protects close-time finalizers.  Keep draining so a
                // broken __gc cannot leak resources owned by later userdata.
            }
        }
    } catch (...) {
        // No exception may escape shutdown, including unexpected corruption
        // in finalizer lookup or queue bookkeeping.
    }
    finalizersRunning_ = false;
}

} // namespace Lua
