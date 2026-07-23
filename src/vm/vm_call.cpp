/**
 * @file vm_call.cpp
 * @brief VM 调用、返回、C 调用与尾调用辅助函数
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

#include <algorithm>
#include <cstdio>
#include <string>

namespace Lua::VM::detail {

namespace {

const char* luaTypeName(const Value& value) {
    switch (value.getType()) {
    case ValueType::Nil:
        return "nil";
    case ValueType::Boolean:
        return "boolean";
    case ValueType::LightUserdata:
        return "userdata";
    case ValueType::Number:
        return "number";
    case ValueType::String:
        return "string";
    case ValueType::Table:
        return "table";
    case ValueType::Function:
        return "function";
    case ValueType::Userdata:
        return "userdata";
    case ValueType::Thread:
        return "thread";
    }
    return "value";
}

Str formatCallTypeError(const Value& value, const Str& callTargetName) {
    const char* typeName = luaTypeName(value);
    if (!callTargetName.empty()) {
        return Str("attempt to call ") + callTargetName + " (a " + typeName + " value)";
    }
    return Str("attempt to call a ") + typeName + " value";
}

bool precallImpl(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults, const Str& callTargetName,
                 CallTargetNameResolver resolver = nullptr, void* resolverContext = nullptr);

} // namespace

void postcall(LuaState* L, i32 funcPos, i32 wantedResults, usize firstResult) {
    Stack& stack = L->getStack();
    const usize resultDestination = static_cast<usize>(funcPos);
    const usize oldTop = L->getAbsoluteTop();
    usize newTop = resultDestination;

    if (firstResult == 0) {
        i32 actualResults = static_cast<i32>(oldTop) - funcPos;
        if (wantedResults >= 0) {
            if (actualResults < wantedResults) {
                while (actualResults < wantedResults) {
                    if (newTop + static_cast<usize>(actualResults) >= stack.size())
                        stack.push(Value());
                    else
                        stack.at(newTop + static_cast<usize>(actualResults)) = Value();
                    actualResults++;
                }
            }
            actualResults = wantedResults;
        }
        newTop = resultDestination + static_cast<usize>(actualResults);
    } else {
        const i32 availableResults = static_cast<i32>(oldTop - firstResult);
        const i32 resultCount = (wantedResults < 0) ? availableResults : wantedResults;
        const i32 copyCount = std::min(availableResults, resultCount);

        /**
         * @brief 按重叠方向安全移动 Lua 返回值。
         *
         * Lua 返回值通常向下覆盖已消费的函数和参数。辅助逻辑需同时适配两个重叠方向，确保所有
         * 保留结果在复制前均不会被覆盖。
         */
        if (resultDestination > firstResult && resultDestination < firstResult + static_cast<usize>(copyCount)) {
            for (i32 i = copyCount; i > 0; --i) {
                stack[resultDestination + static_cast<usize>(i - 1)] = stack[firstResult + static_cast<usize>(i - 1)];
            }
        } else {
            for (i32 i = 0; i < copyCount; ++i) {
                const usize src = firstResult + static_cast<usize>(i);
                const usize dst = resultDestination + static_cast<usize>(i);
                if (shouldDumpBytecode(L)) {
                    std::fprintf(stderr, "[POSTCALL] copy stack[%zu] -> stack[%zu] val=", src, dst);
                    if (stack[src].isNumber())
                        std::fprintf(stderr, "%g", stack[src].asNumber());
                    else if (stack[src].isNil())
                        std::fprintf(stderr, "nil");
                    else
                        std::fprintf(stderr, "other");
                    std::fprintf(stderr, "\n");
                }

                stack[dst] = stack[src];
            }
        }

        for (i32 i = copyCount; i < resultCount; ++i) {
            const usize slot = resultDestination + static_cast<usize>(i);
            if (slot >= stack.size()) {
                stack.push(Value());
            } else {
                stack[slot] = Value();
            }
        }
        newTop = resultDestination + static_cast<usize>(resultCount);
    }

    /**
     * @brief 在降低逻辑栈顶前清理调用者垃圾回收扫描窗口。
     *
     * 结果已稳定存放在目标位置，此时清除仍位于调用者宽扫描窗口内的已消费函数、参数与多余
     * 结果槽。
     */
    const usize clearEnd = std::min(oldTop, stack.size());
    for (usize slot = newTop; slot < clearEnd; ++slot) {
        stack[slot] = Value();
    }

    L->setAbsoluteTop(newTop);

    /**
     * @brief 仅移除 C 调用帧在调用者上方预留的物理尾部栈槽。
     *
     * 此处绝不扩大已返回的 Lua 调用帧，也不缩小到活动调用者寄存器窗口或开放 MULTRET
     * 结果范围以下。
     */
    usize callerFrameTop = newTop;
    const usize currentFrame = L->getCurrentCI();
    LuaVector<CallInfo>& callStack = L->getCallStack();
    if (currentFrame < callStack.size()) {
        const bool calleeStillActive = currentFrame > 0 && callStack[currentFrame].func == resultDestination;
        const usize callerFrame = calleeStillActive ? currentFrame - 1 : currentFrame;
        callerFrameTop = std::max(callerFrameTop, callStack[callerFrame].top);
    }
    if (stack.size() > callerFrameTop) {
        stack.setTop(callerFrameTop);
    }
}

bool precall(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults) {
    return precallImpl(L, funcIndex, nArgs, nResults, Str());
}

bool precallWithName(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults, const Str& callTargetName) {
    return precallImpl(L, funcIndex, nArgs, nResults, callTargetName);
}

bool precallWithNameResolver(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults, CallTargetNameResolver resolver,
                             void* resolverContext) {
    return precallImpl(L, funcIndex, nArgs, nResults, Str(), resolver, resolverContext);
}

namespace {

bool precallImpl(LuaState* L, i32 funcIndex, i32 nArgs, i32 nResults, const Str& callTargetName,
                 CallTargetNameResolver resolver, void* resolverContext) {
    Stack& stack = L->getStack();
    CallInfo& currentCI = L->getCurrentCallInfo();
    usize funcPos = currentCI.base + funcIndex;
    Value funcVal = stack.at(funcPos);

    if (!funcVal.isFunction()) {
        Value tm = getMetamethodByObject(L, funcVal, TMS::TM_CALL);
        if (tm.isNil() || !tm.isFunction()) {
            Str resolvedName = callTargetName;
            if (resolvedName.empty() && resolver) {
                resolvedName = resolver(resolverContext);
            }
            throw RuntimeError(formatCallTypeError(funcVal, resolvedName));
        }

        Value originalFunc = funcVal;
        i32 actualCallArgs = nArgs;
        const bool variableArgCall = actualCallArgs < 0;
        if (variableArgCall) {
            actualCallArgs = static_cast<i32>(L->getAbsoluteTop()) - static_cast<i32>(funcPos + 1);
        }

        LuaVector<Value> args(LuaStdAllocator<Value>(L->getGlobalState().getAllocator()));
        args.reserve(static_cast<usize>(actualCallArgs));
        for (i32 i = 1; i <= actualCallArgs; i++)
            args.push_back(stack.at(funcPos + i));

        while (stack.size() < funcPos + 2 + args.size()) {
            stack.push(Value());
        }
        stack.at(funcPos) = tm;
        stack.at(funcPos + 1) = originalFunc;
        for (usize i = 0; i < args.size(); i++)
            stack.at(funcPos + 2 + i) = args[i];
        nArgs = actualCallArgs + 1;
        if (variableArgCall) {
            L->setAbsoluteTop(funcPos + 1 + static_cast<usize>(nArgs));
        }

        funcVal = stack.at(funcPos);
        if (!funcVal.isFunction()) {
            throw RuntimeError("VM::precall: __call metamethod is not a function");
        }
    }

    Function* func = funcVal.asFunction();

    if (func->isCFunction()) {
        i32 actualNArgs = nArgs;
        if (nArgs < 0) {
            actualNArgs = static_cast<i32>(L->getAbsoluteTop()) - static_cast<i32>(funcPos + 1);
        }

        CallInfo& ci = L->pushCallInfo();
        ci.func = funcPos;
        ci.base = funcPos + 1;
        ci.top = funcPos + 1 + actualNArgs + 20;
        ci.nresults = nResults;
        ci.savedpc = nullptr;
        ci.tailcalls = 0;

        while (stack.size() < ci.top)
            stack.push(Value());
        /**
         * @brief 为更宽的 C 调用帧复用栈槽前清除非参数窗口。
         *
         * 调用返回后 Stack 会保留备用物理栈槽。保守垃圾回收会扫描 ci.top，因此绝不能看到
         * 旧调用帧遗留的指针；这些指针指向的对象可能已经被回收。
         */
        const usize argumentTop = funcPos + 1 + static_cast<usize>(actualNArgs);
        for (usize slot = argumentTop; slot < ci.top; ++slot) {
            stack[slot] = Value();
        }
        L->setAbsoluteTop(funcPos + 1 + actualNArgs);

        dispatchCallHook(L);

        i32 nReturnValues = func->callCFunction(L);

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
        actualArgs = static_cast<i32>(L->getAbsoluteTop()) - static_cast<i32>(funcPos + 1);
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
            compatArgTable = L->getGlobalState().getGC().create<Table>();

            for (i32 i = 0; i < nVarargs; i++) {
                compatArgTable->set(Value(static_cast<LuaNumber>(i + 1)),
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

    while (stack.size() < ci.top)
        stack.push(Value());
    /**
     * @brief 初始化每个非参数寄存器，即使底层 Stack 已因先前调用帧而足够大。
     *
     * 宽范围垃圾回收根扫描会遍历整个 Lua 寄存器窗口；若保留陈旧值，其原对象被清扫后仍会
     * 被误认为存活的垃圾回收指针。
     */
    const usize firstLocal = base + static_cast<usize>(numParams);
    for (usize slot = firstLocal; slot < ci.top; ++slot) {
        stack[slot] = Value();
    }
    if (compatArgTable != nullptr) {
        stack[base + static_cast<usize>(numParams)] = Value(compatArgTable);
    }
    L->setAbsoluteTop(ci.top);

    dispatchCallHook(L);

    return true;
}

} // namespace

void reuseCurrentFrameForTailCall(LuaState* L, usize callerIndex, usize callerFunc, i32 callerTailcalls) {
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
    auto adjustIndex = [offset](usize index) -> usize { return static_cast<usize>(static_cast<i64>(index) + offset); };

    callee.func = adjustIndex(callee.func);
    callee.base = adjustIndex(callee.base);
    callee.top = adjustIndex(callee.top);
    callee.savedpc = nullptr;
    callee.tailcalls = callerTailcalls + 1;
    callee.hookLine = -1;
    callee.hookPc = -1;

    L->popCallInfo();
    L->getCallStack()[callerIndex] = callee;
    stack.setTop(callee.top);
    L->setAbsoluteTop(callee.top);
}

} // namespace Lua::VM::detail
