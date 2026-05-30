/**
 * @file vm_trace.cpp
 * @brief VM trace sink state and debug hook dispatch helpers.
 */

#include "vm/vm_internal.hpp"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "debug/trace_sink.hpp"
#include "debug/trace_types.hpp"
#include "debug/value_serializer.hpp"
#include "vm/state/call_info.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/stack.hpp"
#include "vm/vm.hpp"

#include <format>

namespace Lua {

namespace {

ITraceSink* g_traceSink = nullptr;
u64 g_traceSeq = 0;
bool g_dumpBytecode = false;
bool g_traceDiffEnabled = false;

const char* sourceName(Proto* proto) {
    return proto != nullptr && proto->getSource() != nullptr
               ? proto->getSource()->c_str()
               : "?";
}

Str protoFunctionName(Proto* proto) {
    if (proto == nullptr) {
        return "?";
    }

    const char* source = sourceName(proto);
    const i32 lineDefined = proto->getLineDefined();
    if (lineDefined > 0) {
        return std::format("{}:{}", source, lineDefined);
    }

    return source;
}

Str functionNameFromValue(const Value& value) {
    if (!value.isFunction()) {
        return "?";
    }

    Function* function = value.asFunction();
    if (function == nullptr) {
        return "?";
    }

    if (function->isCFunction()) {
        return "C function";
    }

    return protoFunctionName(function->getProto());
}

Value registerValueAt(LuaState* L, usize frameBase, i32 slot) {
    if (L == nullptr || slot < 0) {
        return Value();
    }

    Stack& stack = L->getStack();
    usize index = frameBase + static_cast<usize>(slot);
    if (index >= stack.size()) {
        return Value();
    }
    return stack[index];
}

TraceEvent makeInstructionEvent(Proto* proto, Value* base, usize instructionPc,
                                Instruction inst, i32 callDepth) {
    TraceEvent event;
    event.seq = g_traceSeq++;
    event.kind = TraceEventKind::Instruction;
    event.pc = static_cast<i32>(instructionPc);
    event.op = GET_OPCODE(inst);
    event.a = GETARG_A(inst);
    event.b = GETARG_B(inst);
    event.c = GETARG_C(inst);
    event.bx = GETARG_Bx(inst);
    event.sbx = GETARG_sBx(inst);
    event.line = proto->getLine(instructionPc);
    event.source = sourceName(proto);
    event.callDepth = callDepth;
    event.funcName = protoFunctionName(proto);
    event.base = base;
    event.maxStack = proto->getMaxStackSize();
    event.proto = proto;
    return event;
}

}  // namespace

namespace VM {

void setTraceSink(ITraceSink* sink) {
    g_traceSink = sink;
    g_traceSeq = 0;
}

ITraceSink* getTraceSink() {
    return g_traceSink;
}

void setTraceDiffEnabled(bool enabled) {
    g_traceDiffEnabled = enabled;
}

bool isTraceDiffEnabled() {
    return g_traceDiffEnabled;
}

}  // namespace VM

namespace VM::detail {

void dispatchCallHook(LuaState* L) {
    if (L->hasDebugHookMask(HookMaskCall)) {
        L->callDebugHook(DebugHookEvent::Call);
    }
}

void dispatchReturnHook(LuaState* L) {
    if (L->hasDebugHookMask(HookMaskReturn)) {
        i32 tailcalls = 0;
        if (L->getCurrentCI() < L->getCallStack().size()) {
            tailcalls = L->getCurrentCallInfo().tailcalls;
        }
        L->callDebugHook(DebugHookEvent::Return);
        while (tailcalls-- > 0) {
            L->callDebugHook(DebugHookEvent::TailReturn);
        }
    }
}

void dispatchCountHook(LuaState* L) {
    if (L->consumeDebugHookCount()) {
        L->callDebugHook(DebugHookEvent::Count);
    }
}

void dispatchLineHook(LuaState* L, Proto* proto, usize pc) {
    if (!L->hasDebugHookMask(HookMaskLine) || L->isDebugHookActive() || proto == nullptr) {
        return;
    }

    i32 line = proto->getLine(pc);
    if (line <= 0) {
        return;
    }

    i32 currentPc = static_cast<i32>(pc);

    CallInfo& ci = L->getCurrentCallInfo();
    if (ci.hookLine == line && ci.hookPc >= 0 && currentPc > ci.hookPc) {
        ci.hookPc = currentPc;
        return;
    }

    ci.hookLine = line;
    ci.hookPc = currentPc;
    L->callDebugHook(DebugHookEvent::Line, line);
}

bool shouldDumpBytecode() {
    return g_dumpBytecode;
}

void emitInstructionTrace(Proto* proto, Value* base, usize instructionPc,
                          Instruction inst, i32 callDepth) {
    if (g_traceSink == nullptr || proto == nullptr) {
        return;
    }

    TraceEvent event = makeInstructionEvent(proto, base, instructionPc, inst, callDepth);

    g_traceSink->onInstruction(event);
}

Vec<Value> captureTraceRegisters(LuaState* L, usize frameBase, i32 maxStack) {
    Vec<Value> snapshot;
    if (L == nullptr || maxStack <= 0) {
        return snapshot;
    }

    snapshot.reserve(static_cast<usize>(maxStack));
    for (i32 slot = 0; slot < maxStack; ++slot) {
        snapshot.push_back(registerValueAt(L, frameBase, slot));
    }
    return snapshot;
}

void emitInstructionTraceDiff(Proto* proto, LuaState* L, usize frameBase, usize instructionPc,
                              Instruction inst, i32 callDepth, const Vec<Value>& before) {
    if (g_traceSink == nullptr || proto == nullptr || L == nullptr) {
        return;
    }

    TraceEvent event = makeInstructionEvent(proto, nullptr, instructionPc, inst, callDepth);
    event.includeChangedRegisters = true;

    i32 maxStack = proto->getMaxStackSize();
    i32 slots = static_cast<i32>(before.size());
    if (maxStack < slots) {
        slots = maxStack;
    }

    for (i32 slot = 0; slot < slots; ++slot) {
        Value after = registerValueAt(L, frameBase, slot);
        const Value& old = before[static_cast<usize>(slot)];
        if (old == after) {
            continue;
        }

        TraceRegisterChange change;
        change.slot = slot;
        if (const char* name = proto->getLocalName(slot + 1, static_cast<i32>(instructionPc))) {
            change.hasName = true;
            change.name = name;
        }
        change.oldValue = Trace::serializeValue(old);
        change.newValue = Trace::serializeValue(after);
        change.oldType = Trace::getValueTypeName(old.getType());
        change.newType = Trace::getValueTypeName(after.getType());
        event.changedRegisters.push_back(std::move(change));
    }

    g_traceSink->onInstruction(event);
}

void emitCallTrace(Proto* proto, Value* base, usize instructionPc,
                   i32 registerIndex, i32 callDepth) {
    if (g_traceSink == nullptr || proto == nullptr || base == nullptr) {
        return;
    }

    TraceEvent event;
    event.seq = g_traceSeq++;
    event.kind = TraceEventKind::Call;
    event.line = proto->getLine(instructionPc);
    event.source = sourceName(proto);
    event.callDepth = callDepth;

    Value& callee = base[registerIndex];
    event.funcName = functionNameFromValue(callee);

    g_traceSink->onCall(event);
}

void emitReturnTrace(Proto* proto, usize instructionPc, i32 callDepth) {
    if (g_traceSink == nullptr || proto == nullptr) {
        return;
    }

    TraceEvent event;
    event.seq = g_traceSeq++;
    event.kind = TraceEventKind::Return;
    event.line = proto->getLine(instructionPc);
    event.source = sourceName(proto);
    event.callDepth = callDepth;
    event.funcName = protoFunctionName(proto);

    g_traceSink->onReturn(event);
}

}  // namespace VM::detail

}  // namespace Lua
