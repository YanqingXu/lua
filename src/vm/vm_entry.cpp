/**
 * @file vm_entry.cpp
 * @brief VM public entry points and C-call bridge.
 */

#include "vm/vm.hpp"

#include "common/lua_error.hpp"
#include "core/function.hpp"
#include "core/value.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/call_info.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm_internal.hpp"

namespace Lua::VM {

namespace {

class HostCallGuard {
public:
    explicit HostCallGuard(LuaState* L) : state_(L) {
        if (state_) {
            state_->enterHostCall();
        }
    }

    ~HostCallGuard() {
        if (state_) {
            state_->leaveHostCall();
        }
    }

    HostCallGuard(const HostCallGuard&) = delete;
    HostCallGuard& operator=(const HostCallGuard&) = delete;

private:
    LuaState* state_;
};

} // namespace

void call(LuaState* L, i32 nargs, i32 nresults) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    call(services, L, nargs, nresults);
}

void call(RuntimeServices& services, LuaState* L, i32 nargs, i32 nresults) {
    services.globalState.requireOwnerThread();
    HostCallGuard hostCall(L);

    usize absTop = L->getAbsoluteTop();
    usize funcPos = absTop - static_cast<usize>(nargs) - 1;

    CallInfo& ci = L->getCurrentCallInfo();
    i32 funcIndex = static_cast<i32>(funcPos - ci.base);

    bool isLua = detail::precall(L, funcIndex, nargs, nresults);
    if (isLua) {
        CallInfo& newCI = L->getCurrentCallInfo();
        Proto* proto = L->getStack()[newCI.func].asFunction()->getProto();
        i32 fpos = static_cast<i32>(newCI.func);
        i32 wantedResults = newCI.nresults;

        executeProto(services, L, proto, 1);

        L->popCallInfo();
        detail::postcall(L, fpos, wantedResults);
    }
}

void execute(LuaState* L, Function* func) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    execute(services, L, func);
}

void execute(RuntimeServices& services, LuaState* L, Function* func) {
    services.globalState.requireOwnerThread();
    if (!func) {
        throw RuntimeError("VM::execute: null function");
    }
    if (func->isCFunction()) {
        throw RuntimeError("VM::execute: C functions not supported yet");
    }

    Stack& stack = L->getStack();

    stack.push(Value(func));
    usize funcIndex = stack.size() - 1;

    CallInfo& ci = L->pushCallInfo();
    ci.func = funcIndex;
    ci.base = funcIndex + 1;
    ci.top = ci.base;
    ci.savedpc = nullptr;
    ci.nresults = -1;
    ci.tailcalls = 0;

    Proto* proto = func->getProto();
    usize requiredTop = ci.base + proto->getMaxStackSize();
    if (stack.capacity() < requiredTop) {
        stack.checkSpace(requiredTop - stack.size());
    }
    while (stack.size() < requiredTop) {
        stack.push(Value());
    }

    ci.top = requiredTop;
    L->setAbsoluteTop(requiredTop);

    detail::dispatchCallHook(L);

    executeProto(services, L, proto, 1);

    L->popCallInfo();
}

} // namespace Lua::VM
