/**
 * @file vm_frame.cpp
 * @brief VM closure and vararg frame helpers.
 */

#include "vm/vm_internal.hpp"

#include "common/lua_error.hpp"
#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/upvalue.hpp"
#include "vm/call_info.hpp"
#include "vm/global_state.hpp"
#include "vm/lua_state.hpp"
#include "vm/stack.hpp"

#include <cstdio>
#include <string>

namespace Lua {
namespace {

Value* refreshBase(LuaState* L) {
    return &L->getStack()[L->getCurrentCallInfo().base];
}

}  // namespace

namespace VM::detail {

void closure(LuaState* L, Value* base, Proto* currentProto, Function* currentFunc,
             usize& pc, i32 a, i32 bx) {
    if (bx < 0 || static_cast<usize>(bx) >= currentProto->getSubProtoCount()) {
        throw RuntimeError("VM: CLOSURE proto index out of range");
    }

    Proto* childProto = currentProto->getSubProto(bx);
    Function* closure = new Function(childProto);
    L->getGlobalState().getGC().registerObject(closure);

    i32 nups = childProto->getNumUpvalues();
    if (nups > 0) {
        const Vec<Instruction>& code = currentProto->getCode();
        const CallInfo& ci = L->getCurrentCallInfo();

        for (i32 j = 0; j < nups; j++) {
            if (pc >= code.size()) {
                throw RuntimeError("VM: CLOSURE missing upvalue pseudo instruction");
            }

            Instruction inst = code[pc++];
            OpCode pop = GET_OPCODE(inst);
            i32 b = GETARG_B(inst);

            if (pop == OpCode::MOVE) {
                closure->addUpvalue(L->findOrCreateUpvalue(ci.base + static_cast<usize>(b)));
            } else if (pop == OpCode::GETUPVAL) {
                Upvalue* uv = currentFunc->getUpvalue(static_cast<usize>(b));
                if (!uv) {
                    throw RuntimeError("VM: CLOSURE invalid parent upvalue index");
                }
                closure->addUpvalue(uv);
            } else {
                throw RuntimeError("VM: CLOSURE expects MOVE/GETUPVAL pseudo instruction");
            }
        }
    }

    base[a] = Value(closure);
}

void vararg(LuaState* L, Value*& base, Proto* proto, i32 a, i32 b) {
    CallInfo& ci = L->getCurrentCallInfo();
    Stack& stack = L->getStack();
    i32 numParams = proto->getNumParams();

    i32 n = static_cast<i32>(ci.base - ci.func - 1) - numParams;
    if (n < 0) n = 0;

    i32 wanted;
    if (b == 0) {
        wanted = n;
        usize neededTop = ci.base + static_cast<usize>(a) + static_cast<usize>(n);
        if (stack.size() < neededTop) {
            stack.setTop(neededTop);
            base = refreshBase(L);
        }
        L->setAbsoluteTop(neededTop);
        if (shouldDumpBytecode()) {
            std::fprintf(stderr, "[VARARG] open multret: wanted=%d neededTop=%zu absTop=%zu stackTop=%zu\n",
                         wanted, neededTop, neededTop, stack.size());
        }
    } else {
        wanted = b - 1;
    }

    for (i32 j = 0; j < wanted; j++) {
        if (j < n) {
            usize srcIndex = ci.base - static_cast<usize>(n) + static_cast<usize>(j);
            if (shouldDumpBytecode()) {
                std::fprintf(stderr, "[VARARG] copy j=%d srcIdx=%zu val=%s\n",
                             j, srcIndex, stack[srcIndex].isNumber()
                                              ? std::to_string(stack[srcIndex].asNumber()).c_str()
                                              : "non-number");
            }
            base[a + j] = stack[srcIndex];
        } else {
            base[a + j] = Value();
        }
    }
}

}  // namespace VM::detail
}  // namespace Lua
