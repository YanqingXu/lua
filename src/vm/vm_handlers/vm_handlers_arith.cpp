/**
 * @file vm_handlers_arith.cpp
 * @brief Arithmetic opcode handlers.
 */

#include "vm/vm_handlers/vm_handler_utils.hpp"

namespace Lua::VM::handlers {

namespace {

HandlerStatus handleArithmetic(OpExecutionContext& context, Instruction inst) {
    LuaState* state = requireState(context);

    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);

    detail::execArithmetic(state, context.proto, context.base, a, b, c, GET_OPCODE(inst));
    return HandlerStatus::Continue;
}

}  // namespace

void registerArithmeticHandlers(HandlerTable& table) noexcept {
    table[opcodeIndex(OpCode::ADD)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::SUB)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::MUL)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::DIV)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::MOD)].handler = handleArithmetic;
    table[opcodeIndex(OpCode::POW)].handler = handleArithmetic;
}

}  // namespace Lua::VM::handlers
