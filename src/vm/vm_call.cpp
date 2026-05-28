/**
 * @file vm_call.cpp
 * @brief VM call, return, C-call, and tailcall helpers.
 */

#include "vm/vm_internal.hpp"

#include "common/lua_error.hpp"
#include "core/function.hpp"
#include "core/metatable.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "vm/state/call_info.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/stack.hpp"

#include <cstdio>
#include <string>

namespace Lua::VM::detail {

void postcall(LuaState* L, i32 funcPos, i32 wantedResults, usize firstResult) {
    Stack& stack = L->getStack();
    usize res = static_cast<usize>(funcPos);
    usize currentTop = L->getAbsoluteTop();

    if (firstResult == 0) {
        i32 actualResults = static_cast<i32>(currentTop) - funcPos;
        if (wantedResults >= 0) {
            if (actualResults < wantedResults) {
                while (actualResults < wantedResults) {
                    if (currentTop >= stack.size()) stack.push(Value());
                    else stack.at(currentTop) = Value();
                    currentTop++;
                    actualResults++;
                }
            } else if (actualResults > wantedResults) {
                currentTop -= (actualResults - wantedResults);
                actualResults = wantedResults;
            }
        }
        L->setAbsoluteTop(funcPos + actualResults);
    } else {
        i32 availableResults = static_cast<i32>(currentTop - firstResult);
        i32 i = (wantedResults < 0) ? availableResults : wantedResults;
        usize src = firstResult;
        while (i != 0 && src < currentTop) {
            if (shouldDumpBytecode()) {
                std::fprintf(stderr, "[POSTCALL] copy stack[%zu] -> stack[%zu] val=", src, res);
                if (stack[src].isNumber()) std::fprintf(stderr, "%g", stack[src].asNumber());
                else if (stack[src].isNil()) std::fprintf(stderr, "nil");
                else std::fprintf(stderr, "other");
                std::fprintf(stderr, "\n");
            }

            stack[res++] = stack[src++];
            i--;
        }
        while (i-- > 0) stack[res++] = Value();
        L->setAbsoluteTop(res);
    }
}

bool precall(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults) {
    Stack& stack = L->getStack();
    CallInfo& currentCI = L->getCurrentCallInfo();
    usize funcPos = currentCI.base + funcIndex;
    Value& funcVal = stack.at(funcPos);

    if (!funcVal.isFunction()) {
        Value tm = getMetamethodByObject(L, funcVal, TMS::TM_CALL);
        if (tm.isNil() || !tm.isFunction()) {
            throw RuntimeError(
                "VM::precall: attempt to call non-function value without __call metamethod at R("
                + std::to_string(funcIndex) + "), abs="
                + std::to_string(funcPos) + ", value=" + funcVal.toString());
        }

        Value originalFunc = funcVal;
        Vec<Value> args;
        for (i32 i = 1; i <= nArgs; i++) args.push_back(stack.at(funcPos + i));

        stack.at(funcPos) = tm;
        stack.at(funcPos + 1) = originalFunc;
        for (usize i = 0; i < args.size(); i++) stack.at(funcPos + 2 + i) = args[i];
        nArgs++;

        funcVal = stack.at(funcPos);
        if (!funcVal.isFunction()) {
            throw RuntimeError("VM::precall: __call metamethod is not a function");
        }
    }

    Function* func = funcVal.asFunction();

    if (func->isCFunction()) {
        CFunction cfunc = func->getCFunction();

        i32 actualNArgs = nArgs;
        if (nArgs < 0) {
            actualNArgs = static_cast<i32>(L->getAbsoluteTop())
                        - static_cast<i32>(funcPos + 1);
        }

        CallInfo& ci = L->pushCallInfo();
        ci.func = funcPos;
        ci.base = funcPos + 1;
        ci.top = funcPos + 1 + actualNArgs + 20;
        ci.nresults = nResults;
        ci.savedpc = nullptr;
        ci.tailcalls = 0;

        while (stack.size() < ci.top) stack.push(Value());
        L->setAbsoluteTop(funcPos + 1 + actualNArgs);

        dispatchCallHook(L);

        i32 nReturnValues = cfunc(L);

        if (L->getStatus() == ThreadStatus::Yield) {
            return false;
        }

        dispatchReturnHook(L);

        usize currentTop = L->getAbsoluteTop();
        usize firstResult = currentTop - static_cast<usize>(nReturnValues);
        postcall(L, static_cast<i32>(funcPos), nResults, firstResult);

        L->popCallInfo();
        return false;
    }

    Proto* proto = func->getProto();
    i32 actualArgs = nArgs;
    if (nArgs < 0) {
        actualArgs = static_cast<i32>(L->getAbsoluteTop())
                   - static_cast<i32>(funcPos + 1);
    }

    i32 numParams = proto->getNumParams();
    usize base;
    Table* compatArgTable = nullptr;

    if (proto->isVararg()) {
        usize oldBase = funcPos + 1;
        usize minArgsTop = oldBase + static_cast<usize>(numParams);
        while (stack.size() < minArgsTop) {
            stack.push(Value());
        }
        for (i32 i = actualArgs; i < numParams; i++) {
            stack[oldBase + static_cast<usize>(i)] = Value();
        }
        if (actualArgs < numParams) {
            actualArgs = numParams;
        }
        i32 nVarargs = actualArgs - numParams;
        if ((proto->getVarargFlags() & VARARG_NEEDSARG) != 0) {
            compatArgTable = new Table();
            L->getGlobalState().getGC().registerObject(compatArgTable);

            for (i32 i = 0; i < nVarargs; i++) {
                compatArgTable->set(
                    Value(static_cast<LuaNumber>(i + 1)),
                    stack[oldBase + static_cast<usize>(numParams + i)]);
            }

            GCString* nKey = L->getGlobalState().getStringPool().intern("n");
            compatArgTable->set(Value(nKey), Value(static_cast<LuaNumber>(nVarargs)));
        }
        base = oldBase + static_cast<usize>(actualArgs);
        usize fixedTop = base + static_cast<usize>(numParams);
        if (stack.size() < fixedTop) {
            stack.setTop(fixedTop);
        }
        for (i32 i = 0; i < numParams; i++) {
            stack[base + static_cast<usize>(i)] = stack[oldBase + static_cast<usize>(i)];
            stack[oldBase + i] = Value();
        }
    } else {
        base = funcPos + 1;
        usize minArgsTop = base + static_cast<usize>(numParams);
        while (stack.size() < minArgsTop) {
            stack.push(Value());
        }
        for (i32 i = actualArgs; i < numParams; i++) {
            stack[base + static_cast<usize>(i)] = Value();
        }
        if (actualArgs < numParams) {
            actualArgs = numParams;
        }
    }

    CallInfo& ci = L->pushCallInfo();
    ci.func = funcPos;
    ci.base = base;
    ci.top = base + proto->getMaxStackSize();
    ci.nresults = nResults;
    ci.savedpc = nullptr;
    ci.tailcalls = 0;

    while (stack.size() < ci.top) stack.push(Value());
    if (compatArgTable != nullptr) {
        stack[base + static_cast<usize>(numParams)] = Value(compatArgTable);
    }
    L->setAbsoluteTop(ci.top);

    dispatchCallHook(L);

    return true;
}

void reuseCurrentFrameForTailCall(LuaState* L, usize callerIndex,
                                  usize callerFunc, i32 callerTailcalls) {
    Stack& stack = L->getStack();
    CallInfo callee = L->getCurrentCallInfo();

    usize src = callee.func;
    usize dst = callerFunc;
    usize count = callee.top - callee.func;

    if (src != dst && count > 0) {
        if (dst < src) {
            for (usize i = 0; i < count; i++) {
                stack[dst + i] = stack[src + i];
            }
        } else {
            for (usize i = count; i > 0; i--) {
                stack[dst + i - 1] = stack[src + i - 1];
            }
        }
    }

    i64 offset = static_cast<i64>(dst) - static_cast<i64>(src);
    auto adjustIndex = [offset](usize index) -> usize {
        return static_cast<usize>(static_cast<i64>(index) + offset);
    };

    callee.func = adjustIndex(callee.func);
    callee.base = adjustIndex(callee.base);
    callee.top = adjustIndex(callee.top);
    callee.savedpc = nullptr;
    callee.tailcalls = callerTailcalls + 1;
    callee.hookLine = -1;

    L->popCallInfo();
    L->getCallStack()[callerIndex] = callee;
    stack.setTop(callee.top);
    L->setAbsoluteTop(callee.top);
}

}  // namespace Lua::VM::detail
