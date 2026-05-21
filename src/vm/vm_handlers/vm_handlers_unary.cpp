/**
 * @file vm_handlers_unary.cpp
 * @brief Unary and concat opcode handlers.
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"

namespace Lua::VM::handlers {

namespace {

HandlerStatus handleUnaryMinus(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Value val = context.base[b];
    Value result;
    detail::unaryMinus(state, result, val);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleNot(OpExecutionContext& context, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    context.base[a] = Value(!context.base[b].isTrue());
    return HandlerStatus::Continue;
}

HandlerStatus handleLength(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);

    Value val = context.base[b];
    Value result;
    detail::length(state, result, val);
    context.base = refreshBase(state);
    context.base[a] = result;
    return HandlerStatus::Continue;
}

HandlerStatus handleConcat(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    detail::concat(context.services, state, context.base, a, b, c);
    context.base = refreshBase(state);
    return HandlerStatus::Continue;
}

}  // namespace

void registerUnaryHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::UNM)].handler = handleUnaryMinus;
    table[opcodeIndex(OpCode::NOT)].handler = handleNot;
    table[opcodeIndex(OpCode::LEN)].handler = handleLength;
    table[opcodeIndex(OpCode::CONCAT)].handler = handleConcat;
}

}  // namespace Lua::VM::handlers
