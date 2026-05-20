/**
 * @file vm_handlers.cpp
 * @brief Initial opcode command handlers used by SwitchDispatch.
 */

#include "vm/vm_handlers.hpp"
#include "common/lua_error.hpp"
#include "core/gc_string.hpp"
#include "core/value.hpp"
#include "core/function.hpp"
#include "vm/vm_internal.hpp"

#include <cstdio>

namespace Lua::VM {

namespace {

usize opcodeIndex(OpCode op) noexcept {
    return static_cast<usize>(op);
}

HandlerStatus handleMove(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    if (VM::detail::shouldDumpBytecode()) {
        std::fprintf(stderr, "[MOVE] pc=%zu a=%d b=%d base[b]=", context.instructionPc, a, b);
        if (context.base[b].isNumber()) std::fprintf(stderr, "%g", context.base[b].asNumber());
        else if (context.base[b].isNil()) std::fprintf(stderr, "nil");
        else if (context.base[b].isFunction()) std::fprintf(stderr, "function");
        else if (context.base[b].isString()) std::fprintf(stderr, "'%s'", context.base[b].asString()->c_str());
        else std::fprintf(stderr, "other");
        std::fprintf(stderr, "\n");
    }

    context.base[a] = context.base[b];
    return HandlerStatus::Continue;
}

HandlerStatus handleLoadK(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 bx = GETARG_Bx(inst);

    context.base[a] = context.proto->getConstant(bx);
    return HandlerStatus::Continue;
}

HandlerStatus handleLoadBool(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    context.base[a] = Value(b != 0);
    if (c != 0) {
        context.pc++;
    }
    return HandlerStatus::Continue;
}

HandlerStatus handleLoadNil(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    for (i32 i = a; i <= b; i++) {
        context.base[i] = Value();
    }
    return HandlerStatus::Continue;
}

HandlerTable makeHandlerTable() {
    HandlerTable table{};

    for (usize index = 0; index < table.size(); ++index) {
        OpCode op = static_cast<OpCode>(index);
        table[index] = HandlerEntry{
            op,
            getOpName(op),
            opcodeGroup(op),
            nullptr
        };
    }

    table[opcodeIndex(OpCode::MOVE)].handler = handleMove;
    table[opcodeIndex(OpCode::LOADK)].handler = handleLoadK;
    table[opcodeIndex(OpCode::LOADBOOL)].handler = handleLoadBool;
    table[opcodeIndex(OpCode::LOADNIL)].handler = handleLoadNil;

    return table;
}

}  // namespace

const HandlerTable& handlerTable() noexcept {
    static const HandlerTable table = makeHandlerTable();
    return table;
}

OpHandler handlerFor(OpCode op) noexcept {
    usize index = opcodeIndex(op);
    const HandlerTable& table = handlerTable();
    if (index >= table.size()) {
        return nullptr;
    }
    return table[index].handler;
}

bool hasHandler(OpCode op) noexcept {
    return handlerFor(op) != nullptr;
}

HandlerStatus runHandler(OpExecutionContext& context, Instruction inst) {
    OpCode op = GET_OPCODE(inst);
    OpHandler handler = handlerFor(op);
    if (!handler) {
        throw RuntimeError("VM: no handler registered for opcode: " + Str(getOpName(op)));
    }
    return handler(context, inst);
}

}  // namespace Lua::VM
