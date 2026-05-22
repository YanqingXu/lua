/**
 * @file vm_trace.cpp
 * @brief VM trace sink state and debug hook dispatch helpers.
 */

#include "vm/vm_internal.hpp"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "debug/trace_sink.hpp"
#include "debug/trace_types.hpp"
#include "vm/state/call_info.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

namespace Lua {

namespace {

ITraceSink* g_traceSink = nullptr;
u64 g_traceSeq = 0;
bool g_dumpBytecode = false;

const char* sourceName(Proto* proto) {
    return proto != nullptr && proto->getSource() != nullptr
               ? proto->getSource()->c_str()
               : "?";
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

}  // namespace VM

namespace VM::detail {

void dispatchCallHook(LuaState* L) {
    if (L->hasDebugHookMask(HookMaskCall)) {
        L->callDebugHook(DebugHookEvent::Call);
    }
}

void dispatchReturnHook(LuaState* L) {
    if (L->hasDebugHookMask(HookMaskReturn)) {
        L->callDebugHook(DebugHookEvent::Return);
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

    CallInfo& ci = L->getCurrentCallInfo();
    if (ci.hookLine == line) {
        return;
    }

    ci.hookLine = line;
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
    event.base = base;
    event.maxStack = proto->getMaxStackSize();
    event.proto = proto;

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
    if (callee.isFunction()) {
        Function* calleeFunction = callee.asFunction();
        if (calleeFunction->getProto() != nullptr &&
            calleeFunction->getProto()->getSource() != nullptr) {
            event.funcName = calleeFunction->getProto()->getSource()->c_str();
        }
    }

    g_traceSink->onCall(event);
}

void emitReturnTrace(i32 callDepth) {
    if (g_traceSink == nullptr) {
        return;
    }

    TraceEvent event;
    event.seq = g_traceSeq++;
    event.kind = TraceEventKind::Return;
    event.callDepth = callDepth;

    g_traceSink->onReturn(event);
}

}  // namespace VM::detail

}  // namespace Lua
