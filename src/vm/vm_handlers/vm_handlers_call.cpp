/**
 * @file vm_handlers_call.cpp
 * @brief Call-family opcode handlers.
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"
#include "core/gc_string.hpp"
#include "vm/state/call_info.hpp"

#include <cstdio>

namespace Lua::VM::handlers {

namespace {

HandlerStatus handleCall(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);
    i32 nArgs = b - 1;
    i32 nResults = c - 1;

    if (detail::shouldDumpBytecode()) {
        CallInfo& dbgCI = state->getCurrentCallInfo();
        std::fprintf(stderr, "[CALL] pc=%zu a=%d B=%d C=%d nArgs=%d nRes=%d base=%zu absTop=%zu\n",
                     context.instructionPc, a, b, c, nArgs, nResults, dbgCI.base,
                     state->getAbsoluteTop());
        if (nArgs < 0) {
            usize funcP = dbgCI.base + static_cast<usize>(a);
            Stack& dbgStk = state->getStack();
            for (usize si = funcP; si < state->getAbsoluteTop(); si++) {
                Value& v = dbgStk[si];
                if (v.isNumber()) std::fprintf(stderr, "  [%zu] number=%g\n", si, v.asNumber());
                else if (v.isFunction()) std::fprintf(stderr, "  [%zu] function\n", si);
                else if (v.isString()) std::fprintf(stderr, "  [%zu] string='%s'\n", si, v.asString()->c_str());
                else if (v.isNil()) std::fprintf(stderr, "  [%zu] nil\n", si);
                else std::fprintf(stderr, "  [%zu] other\n", si);
            }
        }
    }

    detail::emitCallTrace(proto, context.base, context.instructionPc, a, context.nexeccalls + 1);

    const auto code = proto->getInstructionSpan();
    state->getCurrentCallInfo().savedpc = code.data() + context.pc;

    bool isLua = detail::precall(state, a, nArgs, nResults);

    if (isLua) {
        context.nexeccalls++;
        return HandlerStatus::Reenter;
    }

    if (state->getStatus() == ThreadStatus::Yield) {
        state->setSavedNexeccalls(context.nexeccalls);
        return HandlerStatus::Yielded;
    }

    CallInfo& callerCI = state->getCurrentCallInfo();
    Stack& stack = state->getStack();
    if (nResults >= 0) {
        stack.setTop(callerCI.top);
        state->setAbsoluteTop(callerCI.top);
    }

    context.base = refreshBase(state);
    return HandlerStatus::Continue;
}

HandlerStatus handleTailCall(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 nArgs = b - 1;

    usize callerIndex = state->getCurrentCI();
    CallInfo& currentCI = state->getCurrentCallInfo();
    usize callerFunc = currentCI.func;
    i32 callerTailcalls = currentCI.tailcalls;
    state->closeUpvalues(currentCI.base);

    const auto code = proto->getInstructionSpan();
    currentCI.savedpc = code.data() + context.pc;

    bool isLua = detail::precall(state, a, nArgs, -1);

    if (isLua) {
        detail::reuseCurrentFrameForTailCall(state, callerIndex, callerFunc, callerTailcalls);
        return HandlerStatus::Reenter;
    }

    context.base = refreshBase(state);
    return HandlerStatus::Continue;
}

HandlerStatus handleReturn(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);
    Proto* proto = requireProto(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    detail::emitReturnTrace(proto, context.instructionPc, context.nexeccalls);
    detail::dispatchReturnHook(state);
    context.base = refreshBase(state);

    CallInfo& ci = state->getCurrentCallInfo();
    Stack& stack = state->getStack();

    state->closeUpvalues(ci.base);

    i32 nres;
    if (b == 0) {
        nres = static_cast<i32>(state->getAbsoluteTop())
             - (static_cast<i32>(ci.base) + a);
    } else {
        nres = b - 1;
    }

    for (i32 i = 0; i < nres; i++) {
        stack.at(ci.func + static_cast<usize>(i)) = context.base[a + i];
    }

    usize newTop = ci.func + static_cast<usize>(nres);
    while (stack.size() > newTop) {
        stack.pop();
    }
    state->setAbsoluteTop(newTop);

    context.nexeccalls--;
    if (context.nexeccalls == 0) {
        return HandlerStatus::Returned;
    }

    i32 funcPos = static_cast<i32>(ci.func);
    i32 wantedResults = ci.nresults;
    state->popCallInfo();
    detail::postcall(state, funcPos, wantedResults);
    return HandlerStatus::Reenter;
}

}  // namespace

void registerCallHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::CALL)].handler = handleCall;
    table[opcodeIndex(OpCode::TAILCALL)].handler = handleTailCall;
    table[opcodeIndex(OpCode::RETURN)].handler = handleReturn;
}

}  // namespace Lua::VM::handlers
